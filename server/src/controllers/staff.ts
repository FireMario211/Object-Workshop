import { getCache } from '../postgres';
import { Router, Request, Response } from 'express';
import { body, param, validationResult } from 'express-validator';
import { UserData } from '@/Components/User';
import { verifyToken } from './user';
import moment from 'moment'

const sRouter = Router();

function banToRole(type: number): number {
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
                    if (userData.role != 3) return res.status(403).json({error: "No permission"})
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
                        INSERT INTO cases (case_type, account_id, staff_account_id, reason${(requiresExp) ? ", expiration" : ""})
                        VALUES ($1, $2, $3, $4${(requiresExp) ? ", $5" : ""})
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
        getCache().then(pool => {
            verifyToken(pool, token).then(verifyRes => {
                if (!verifyRes.valid && verifyRes.expired) {
                    return res.status(410).json({ error: verifyRes.message });
                } else if (!verifyRes.valid) {
                    return res.status(401).json({ error: verifyRes.message });
                }
                if (verifyRes.user) {
                    let user = verifyRes.user;
                    pool.query("SELECT case_id,case_type,account_id,reason,timestamp,ack FROM cases WHERE account_id = $1 AND case_id = $2 AND case_type = 0 AND ack = FALSE", [user.account_id, caseID]).then(caseRes => {
                        if (!caseRes.rows.length) return res.status(404).json({error: "Case not found."});
                        pool.query("UPDATE cases SET ack = TRUE, ack_timestamp = $1 WHERE case_id = $2", [new Date(), caseID]).then(() => {
                            res.status(200).json({message: "Acknowledged"});
                        }).catch(e => {
                            console.error(e)
                            res.status(500).json({ error: 'Internal server error' });
                        });
                    }).catch(e => {
                        console.error(e)
                        res.status(500).json({ error: 'Internal server error' });
                    })
                }
            }).catch(() => {
                res.status(500).json({ error: 'Internal server error' });
            })
        }).catch(e => {
            console.error(e);
            res.status(500).json({ error: 'Internal server error' });
        });
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
    body('role').isInt({min: -5, max: 5}).notEmpty(),
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

export default sRouter;
