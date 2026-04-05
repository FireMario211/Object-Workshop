import { getCache } from '../postgres';
import { Router, Request, Response } from 'express';
import { body, param, validationResult } from 'express-validator';
import { UserData } from '@/Components/User';
import { verifyToken } from './user';
import moment from 'moment'

const sRouter = Router();

export function banToRole(type: number): number {
    //return [0, -2, -1, -3, -2, -1, -3][type]
    switch (type) {
        default: return 0;
        case 1:
        case 4:
            return -1;
        case 2:
        case 5:
            return -2;
        case 3:
        case 6:
            return -3;
    }
}

export function caseTypeToName(type: number, color: boolean): string {
    let name: string = "Unknown";
    let ccolor: string = "cg";
    switch (type) {
        case 0:
            name = "Warning";
            ccolor = "cy";
            break;
        case 1:
            name = "Upload Temp-Ban";
            ccolor = "cb";
            break;
        case 2:
            name = "Comment Temp-Ban";
            ccolor = "cb";
            break;
        case 3:
            name = "Account Temp-Ban";
            ccolor = "cb";
            break;
        case 4:
            name = "Upload Ban";
            ccolor = "cr";
            break;
        case 5:
            name = "Comment Ban";
            ccolor = "cr";
            break;
        case 6:
            name = "Account Ban";
            ccolor = "cr";
            break;
    }
    if (color) {
        return `<${ccolor}>${name}</c>`
    } else {
        return name;
    }
}

// type, 0 = warn, 1 = tempban (comment), 2 = tempban (uploading), 3 = tempban (everything), 4 = ban (comment), 5 = ban (uploading), 6 = ban (everything)
sRouter.post("/case/create", 
    body('token').notEmpty().isString().withMessage("Token is required"),
    body('user').isInt({min: 0, max: 2147483647}).notEmpty(),
    body('type').isInt({min: 0, max: 6}).notEmpty(),
    body('reason').notEmpty().isString(),
    body('expiration').optional().isString(),
    async (req: Request, res: Response) => {
        const result = validationResult(req);
        if (!result.isEmpty()) return res.status(400).json({ errors: result.array() })
        const token = req.body.token as string;
        getCache().then(pool => {
            verifyToken(pool, token).then(async verifyRes => {
                if (!verifyRes.valid && verifyRes.expired) {
                    return res.status(410).json({ error: verifyRes.message });
                } else if (!verifyRes.valid) {
                    return res.status(401).json({ error: verifyRes.message });
                }
                if (verifyRes.user) {
                    const userData = verifyRes.user;
                    if (userData.role < 2) return res.status(403).json({error: "No permission"})
                    const userID = parseInt(req.body.user as string);
                    const caseType = parseInt(req.body.type as string);
                    const reason = req.body.reason;
                    if (reason.length > 500) return res.status(413).json({error: "The reason cannot be more than 500 characters long!"});
                    const expiration = moment(new Date(req.body.expiration));
                    const requiresExp = [1,2,3].includes(caseType);
                    if (!expiration.isValid() && requiresExp) return res.status(413).json({error: "how are you supposed to temp punish someone if you dont provide an expiration??"});
                    if (expiration.isValid() && requiresExp) {
                        const monthCompare = expiration.month() - moment().month();
                        if (monthCompare < 0) return res.status(413).json({error: "what??? how can you have the expiration be -1 months?"});
                        if (monthCompare > 12) return res.status(413).json({error: "thats a bit too long..."});
                    }
                    const userResult = await pool.query("SELECT account_id,name,timestamp,role,ban_reason FROM users WHERE account_id = $1", [userID]);
                    if (userResult.rows.length == 0) return res.status(404).json({error: "User not found."});
                    const targetUserData = userResult.rows[0] as UserData;
                    if (targetUserData.role >= userData.role) return res.status(403).json({error: "You cannot punish a user that has the same role or higher than you!"});
                    const query = `
                        INSERT INTO cases (case_type, account_id, staff_account_id, reason${(requiresExp) ? ", expiration" : ""}, notice_ack)
                        VALUES ($1, $2, $3, $4${(requiresExp) ? ", $5" : ""}, false)
                    `;
                    const values = [caseType, userID, userData.account_id, reason];
                    if (requiresExp) values.push(expiration);
                    await pool.query(query, values);
                    if (caseType > 0) {
                        await pool.query('UPDATE users SET role = $1, ban_reason = $2 WHERE account_id = $3', [banToRole(caseType), reason, userID]);
                    }
                    res.status(200).json({ message: "Case submitted." })
                }
            }).catch(e => {
                console.error(e);
                res.status(500).json({ error: 'Internal server error' });
            })
        }).catch(e => {
            console.error(e);
            res.status(500).json({ error: 'Internal server error' });
        });
    }
)

sRouter.post("/case/:id/ack",
    param('id').isInt({min: 0, max: 2147483647}).notEmpty(),
    body('token').notEmpty().isString().withMessage("Token is required"),
    async (req: Request, res: Response) => {
        const result = validationResult(req);
        if (!result.isEmpty()) return res.status(400).json({ errors: result.array() })
        const token = req.body.token as string;
        const caseID = parseInt(req.params.id as string);
        try {
            const pool = await getCache();
            const verifyRes = await verifyToken(pool, token);
            if (!verifyRes.valid && verifyRes.expired) {
                return res.status(410).json({ error: verifyRes.message });
            } else if (!verifyRes.valid) {
                return res.status(401).json({ error: verifyRes.message });
            }
            if (!verifyRes.user) return res.status(500).json({ error: 'Internal server error' });
            const user = verifyRes.user;
            const caseRes = await pool.query(
                `SELECT case_id, case_type, expiration, ack, notice_ack FROM cases WHERE account_id = $1 AND case_id = $2 AND (
                    (case_type = 0 AND ack = FALSE)
                    OR (case_type IN (1, 2, 3) AND expiration IS NOT NULL AND expiration > NOW() AND COALESCE(notice_ack, false) = FALSE)
                    OR (case_type IN (1, 2, 3) AND expiration IS NOT NULL AND expiration <= NOW() AND ack = FALSE)
                    OR (case_type IN (4, 5, 6) AND COALESCE(notice_ack, false) = FALSE)
                )`,
                [user.account_id, caseID]
            );
            if (!caseRes.rows.length) return res.status(404).json({ error: "Case not found." });
            const row = caseRes.rows[0];
            const now = new Date();
            if (row.case_type === 0) {
                await pool.query("UPDATE cases SET ack = TRUE, ack_timestamp = $1 WHERE case_id = $2", [now, caseID]);
            } else if ([1, 2, 3].includes(row.case_type)) {
                const exp = moment(row.expiration);
                if (exp.isAfter(moment())) {
                    await pool.query("UPDATE cases SET notice_ack = TRUE WHERE case_id = $1", [caseID]);
                } else {
                    await pool.query("UPDATE cases SET ack = TRUE, ack_timestamp = $1 WHERE case_id = $2", [now, caseID]);
                }
            } else if ([4, 5, 6].includes(row.case_type)) {
                await pool.query("UPDATE cases SET notice_ack = TRUE WHERE case_id = $1", [caseID]);
            }
            return res.status(200).json({ message: "Acknowledged" });
        } catch (e) {
            console.error(e);
            return res.status(500).json({ error: 'Internal server error' });
        }
    }
);

sRouter.post("/user/:id/cases",
    param('id').isInt({min: 0, max: 2147483647}).notEmpty(),
    body('token').notEmpty().isString().withMessage("Token is required"),
    async (req: Request, res: Response) => {
        const result = validationResult(req);
        if (!result.isEmpty()) return res.status(400).json({ errors: result.array() })
        const token = req.body.token as string;
        const accountID = parseInt(req.params.id as string);
        getCache().then(pool => {
            verifyToken(pool, token).then(verifyRes => {
                if (!verifyRes.valid && verifyRes.expired) {
                    return res.status(410).json({ error: verifyRes.message });
                } else if (!verifyRes.valid) {
                    return res.status(401).json({ error: verifyRes.message });
                }
                if (verifyRes.user) {
                    let user = verifyRes.user;
                    if (user.role < 2) return res.status(403).json({error: "No permission"})
                    pool.query(`
                        SELECT
                            c.*, 
                            u.name as staff_account_name,
                            COUNT(*) OVER() AS total_records
                        FROM
                            cases c
                        JOIN
                            users u ON c.staff_account_id = u.account_id
                        WHERE c.account_id = $1
                        GROUP BY c.case_id, u.name
                        ORDER BY c.timestamp DESC
                    `, [accountID]
                    ).then(caseRes => {
                        if (!caseRes.rows.length) return res.status(404).json({error: "No cases found."});
                        const totalRecords = (caseRes.rows.length > 0) ? parseInt(caseRes.rows[0].total_records) : 0;
                        res.status(200).json({
                            results: caseRes.rows.map(row => {
                                row.timestamp = moment(row.timestamp).format();
                                row.expiration = moment(row.expiration).format();
                                row.ack_timestamp = moment(row.ack_timestamp).format();
                                return row;
                            }),
                            total: totalRecords
                        });
                    }).catch(e => {
                        console.error(e)
                        res.status(500).json({ error: 'Internal server error' });
                    })
                }
            }).catch(e => {
                console.error(e);
                res.status(500).json({ error: 'Internal server error' });
            })
        }).catch(e => {
            console.error(e);
            res.status(500).json({ error: 'Internal server error' });
        });
    }
);

sRouter.post("/user/:id/role",
    param('id').isInt({min: 0, max: 2147483647}).notEmpty(),
    body('role').isInt({min: -3, max: 4}).notEmpty(),
    body('token').notEmpty().isString().withMessage("Token is required"),
    async (req: Request, res: Response) => {
        const result = validationResult(req);
        if (!result.isEmpty()) return res.status(400).json({ errors: result.array() })
        const token = req.body.token as string;
        const accountID = parseInt(req.params.id as string, 10);
        const newRole = parseInt(req.body.role as string, 10);
        try {
            const pool = await getCache();
            const verifyRes = await verifyToken(pool, token);
            if (!verifyRes.valid && verifyRes.expired) {
                return res.status(410).json({ error: verifyRes.message });
            } else if (!verifyRes.valid) {
                return res.status(401).json({ error: verifyRes.message });
            }
            if (!verifyRes.user) return res.status(500).json({ error: 'Internal server error' });
            const actor = verifyRes.user;
            if (actor.role < 3) {
                return res.status(403).json({ error: "No permission" });
            }
            if (actor.account_id === accountID) {
                return res.status(403).json({ error: "You cannot change your own role." });
            }
            const targetResult = await pool.query(
                "SELECT account_id, name, role FROM users WHERE account_id = $1",
                [accountID]
            );
            if (targetResult.rows.length === 0) {
                return res.status(404).json({ error: "User not found." });
            }
            const target = targetResult.rows[0] as UserData;
            if (target.role >= actor.role) {
                return res.status(403).json({ error: "You cannot change a user with the same role or higher than you." });
            }
            if (newRole > actor.role) {
                return res.status(403).json({ error: "You cannot assign a role higher than your own." });
            }
            await pool.query("UPDATE users SET role = $1 WHERE account_id = $2", [newRole, accountID]);
            return res.sendStatus(200);
        } catch (e) {
            console.error(e);
            return res.status(500).json({ error: 'Internal server error' });
        }
    }
);

export default sRouter;
