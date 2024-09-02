import { getCache } from '../postgres';
import { Router, Request, Response } from 'express';
import { body, query, validationResult } from 'express-validator';
import crypto from 'crypto'
import axios from 'axios';
import { UserData } from '@/Components/User';
import { PoolClient } from 'pg';

const uRouter = Router();

function generateAuthToken(account_id: string): string {
    // what are the odds for a collision? 2^256!? 10^77!? wait... THIS IS THE SAME NUMBER!
    return crypto.createHash('sha256').update(crypto.randomBytes(32).toString('hex') + account_id).digest('hex');
    //return crypto.randomBytes(32).toString('hex') + account_id;
}

interface VerifyTokenResult {
    valid: boolean;
    expired: boolean;
    message: string;
    user?: UserData;
}

export async function verifyToken(pool: PoolClient, token: string): Promise<VerifyTokenResult> {
    try {
        const result = await pool.query("SELECT * FROM user_tokens WHERE token = $1", [token]);
        if (result.rows.length == 0) return { valid: false, expired: false, message: 'Invalid token.' };
        const userToken = result.rows[0];

        const currentTime = new Date();
        if (userToken.expiration < currentTime) {
            await pool.query("DELETE FROM user_tokens WHERE token_id = $1", [userToken.token_id]);
            return { valid: false, expired: true, message: 'Token expired.' };
        }

        const userResult = await pool.query("SELECT * FROM users WHERE account_id = $1", [userToken.account_id]);
        if (userResult.rows.length == 0) return { valid: false, expired: false, message: 'User not found.' };
        const user = userResult.rows[0];
        return { valid: true, expired: false, message: 'Valid token', user };
    } catch (e) {
        console.error('Error verifying token:', e);
        throw new Error("Internal server error.");
    }
}

interface GDAuth {
    sessionID: string,
	accountID: number;
    username: string;
    expires_after: Date;
};

uRouter.get("/testverify", query('token').notEmpty().isUUID(4), async (req: Request, res: Response) => {
    const result = validationResult(req);
    if (!result.isEmpty()) return res.status(400).json({ errors: result.array() })
    axios.post("https://gd.figm.io/authentication/validate", `sessionID=${req.query.token}`).then(axiosRes => {
        res.status(200).json(axiosRes.data);
    })
})

uRouter.post("/user/@me",
    body('token').notEmpty().isString().withMessage("Token is required"),
    async (req: Request, res: Response) => {
        const result = validationResult(req);
        if (!result.isEmpty()) return res.status(400).json({ errors: result.array() })
        const token = req.body.token as string;
        getCache().then(pool => {
            verifyToken(pool, token).then(verifyRes => {
                if (!verifyRes.valid && verifyRes.expired) {
                    return res.status(410).json({ error: verifyRes.message });
                } else if (!verifyRes.valid) {
                    return res.status(401).json({ error: verifyRes.message });
                }
                if (verifyRes.user) {
                    let user = verifyRes.user;
                    pool.query("SELECT count(*) AS count FROM objects WHERE account_id = $1", [user.account_id]).then(objectRes => {
                        user.uploads = parseInt(objectRes.rows[0].count.toString());
                        res.status(200).json(user);
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

uRouter.post('/verify',
    body('token').notEmpty().isString().withMessage("Token is required"),
    async (req: Request, res: Response) => {
        const result = validationResult(req);
        if (!result.isEmpty()) return res.status(400).json({ errors: result.array() })
        const token = req.body.token as string;
        getCache().then(pool => {
            verifyToken(pool, token).then(verifyRes => {
                if (!verifyRes.valid && verifyRes.expired) {
                    return res.status(410).json({ error: verifyRes.message });
                } else if (!verifyRes.valid) {
                    return res.status(401).json({ error: verifyRes.message });
                }
                res.status(200).json({ message: verifyRes.message });
            }).catch(() => {
                res.status(500).json({ error: 'Internal server error' });
            })
        }).catch(e => {
            console.error(e);
            res.sendStatus(500);
        });
    }
);

uRouter.post('/gdauth',
    body('token').notEmpty().isUUID(4),
    async (req: Request, res: Response) => {
        const result = validationResult(req);
        if (!result.isEmpty()) return res.status(400).json({ errors: result.array() })
        const token = req.body.token as string;
        getCache().then(pool => {
            // ok WHY DOES FIGS SERVER NOT ALLOW JSON!?!? so dumb that i have to do this!
            axios.post("https://gd.figm.io/authentication/validate", `sessionID=${token}`).then(axiosRes => {
                const data = axiosRes.data as GDAuth;
                console.log("[GDAuth]", data);
                if (axiosRes.data == "-1") return res.status(403).json({error: "Invalid token."});
                pool.query("INSERT INTO users (auth_method, account_id, name) VALUES ($1, $2, $3) ON CONFLICT (account_id) DO NOTHING RETURNING *;", ["gdauth", data.accountID, data.username]).then(async userResult => {
                    let user = userResult.rows[0];
                    if (!user) {
                        const existingUserResult = await pool.query(
                            `SELECT * FROM users WHERE account_id = $1;`,
                            [data.accountID]
                        );
                        user = existingUserResult.rows[0];
                    }
                    if (!user) return res.status(500).json({ error: "Unable to retrieve or create user." });
                    const authToken = generateAuthToken(user.account_id);
                    const expiration = new Date(Date.now() + ((60 * 60 * 1000) * 24) * 3); // 3 days
                    await pool.query(
                        `INSERT INTO user_tokens (account_id, token, expiration)
                         VALUES ($1, $2, $3);`,
                        [user.account_id, authToken, expiration]
                    );
                    res.status(200).json({ token: authToken });
                }).catch(err => {
                    console.error(err);
                    res.status(500).json({ error: "Something went wrong when trying to create a user." })
                })

            }).catch(e => {
                console.error(e);
                res.status(500).send({error: "Something went wrong when trying to communicate with GDAuth servers."})
            })
            //pool.query("SELECT EXISTS (SELECT 1 FROM users WHERE account_id = $1)", [accountID]).then(resp => {
            //    if (resp.rows[0].exists) {
        }).catch(e => {
            console.error(e);
            res.sendStatus(500);
        });
    }
);

uRouter.post('/custom',
    body('token').notEmpty().isString(),
    body('accountID').notEmpty().isString(),
    body('username').notEmpty().isString(),
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
                if (verifyRes.user && verifyRes.user.role != 3) return res.status(403).json({ error: "No permission" });
                const data = req.body;
                pool.query("INSERT INTO users (auth_method, account_id, name) VALUES ($1, $2, $3) ON CONFLICT (account_id) DO NOTHING RETURNING *;", ["custom", data.accountID, data.username]).then(async userResult => {
                    let user = userResult.rows[0];
                    if (!user) {
                        const existingUserResult = await pool.query(
                            `SELECT * FROM users WHERE account_id = $1;`,
                            [data.accountID]
                        );
                        user = existingUserResult.rows[0];
                    }
                    if (!user) return res.status(500).json({ error: "Unable to retrieve or create user." });
                    const authToken = generateAuthToken(user.account_id);
                    const expiration = new Date(Date.now() + ((60 * 60 * 1000) * 24) * 365); // 1 year
                    await pool.query(
                        `INSERT INTO user_tokens (account_id, token, expiration)
                         VALUES ($1, $2, $3);`,
                        [user.account_id, authToken, expiration]
                    );
                    res.status(200).json({ token: authToken });
                }).catch(err => {
                    console.error(err);
                    res.status(500).json({ error: "Something went wrong when trying to create a user." })
                })
            }).catch(() => {
                res.status(500).json({ error: 'Internal server error' });
            })
        }).catch(e => {
            console.error(e);
            res.sendStatus(500);
        });
    }
);

export default uRouter;
