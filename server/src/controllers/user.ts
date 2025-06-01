import { getCache } from '../postgres';
import { Router, Request, Response } from 'express';
import { body, param, CustomValidator, query, validationResult } from 'express-validator';
import crypto from 'crypto'
import axios from 'axios';
import { UserData } from '@/Components/User';
import { PoolClient } from 'pg';
import moment from 'moment'


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

export function banCheck(res: Response, user: UserData, type: number): boolean {
    if (user.role < 0) {
        if (user.role == -1 && type == 1) {
            res.status(403).json({error: "You are banned from uploading and reporting! Reason: " + user.ban_reason});
            return true;
        }
        if (user.role == -2 && type == 2) {
            res.status(403).json({error: "You are banned from commenting! Reason: " + user.ban_reason});
            return true;
        }
        if (user.role == -3) {
            res.status(403).json({error: "You are banned! Reason: " + user.ban_reason});
            return true;
        }
        return false;
    } else {
        return false;
    }
}

interface GDAuth {
    sessionID: string,
	accountID: number;
    username: string;
    expires_after: Date;
};

interface DashAuth {
    success: boolean,
    message: string,
    data: {
        id: number,
        username: string,
        token: string,
        token_expiration: Date
    }
};

// {"valid":true,"valid_weak":true,"username":"FireeDev"}
interface Argon {
    valid: boolean,
    valid_weak: boolean,
    cause?: string,
    username: string
};

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
                        pool.query("SELECT case_id,case_type,account_id,reason,timestamp,ack FROM cases WHERE account_id = $1 AND case_type = 0", [user.account_id]).then(caseRes => {
                            if (caseRes.rows.length > 0) {
                                const cases = caseRes.rows.filter(data => data.ack == false);
                                res.status(200).json({
                                    ...user,
                                    cases: cases,
                                    warning: caseRes.rows.length
                                })
                            } else {
                                res.status(200).json(user);
                            }
                            pool.query("SELECT case_id,case_type,account_id,reason,timestamp,ack,expiration FROM cases WHERE account_id = $1 AND case_type != 0 AND ack = FALSE", [user.account_id]).then(otherCaseRes => {
                                if (otherCaseRes.rows.length > 0 && user.account_id < 0) {
                                    const cases = otherCaseRes.rows.filter(data => [1,2,3].includes(data.case_type)).map(x => {
                                        x.expiration = moment(x.expiration);
                                        return x;
                                    });
                                    const currentDate = moment();
                                    if (cases.length) {
                                        cases.forEach(caseData => {
                                            if (currentDate.isAfter(caseData.expiration)) {
                                                pool.query("UPDATE cases SET ack = TRUE, ack_timestamp = $1 WHERE case_id = $2", [new Date(), caseData.case_id])
                                                pool.query("UPDATE users SET role = 0 WHERE account_id = $1", [user.account_id]);
                                                console.log(`${user.account_id} was unpunished due to it being expired! Case ID ${caseData.case_id}`);
                                            }
                                        })
                                    }
                                }
                            })
                        }).catch(e => {
                            console.error(e)
                            res.status(500).json({ error: 'Internal server error' });
                        })
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

const areValidIcon: CustomValidator = (iconSet: string[]) => {
    return iconSet.length == 5 && iconSet.map(x => !isNaN(parseInt(x)));
};

// icon,playerColor,playerColor2,playerColorGlow,glow
uRouter.post('/icon',
    body('token').notEmpty().isString().withMessage("Token is required"),
    body('icon').notEmpty().isArray().custom(areValidIcon),
    body('icon.*').isInt({min: 0, max: 100000}),
    async (req: Request, res: Response) => {
        const result = validationResult(req);
        if (!result.isEmpty()) return res.status(400).json({ errors: result.array() })
        const token = req.body.token as string;
        let icon = req.body.icon as Array<number>;
        if (!icon || !icon.length) icon = [];
        if (icon.length > 5) return res.status(413).json({error: "You can only add a maximum of 5 icons!"});
        getCache().then(pool => {
            verifyToken(pool, token).then(async verifyRes => {
                if (!verifyRes.valid && verifyRes.expired) {
                    return res.status(410).json({ error: verifyRes.message });
                } else if (!verifyRes.valid) {
                    return res.status(401).json({ error: verifyRes.message });
                }
                if (!verifyRes.user) return res.sendStatus(500);
                try {
                    await pool.query(
                        `UPDATE users SET icon = $1 WHERE account_id = $2`,
                        [icon, verifyRes.user.account_id]
                    );
                    res.status(200).json({ message: "Updated icon" });
                } catch (e) {
                    console.error(e);
                    res.status(500).json({ error: 'Internal server error' });
                }
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
                if (!process.env.PRODUCTION) {
                    console.log("[GDAuth]", data);
                }
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
                    const expiration = new Date(Date.now() + ((60 * 60 * 1000) * 24) * 7); // 1 week
                    await pool.query(
                        `INSERT INTO user_tokens (account_id, token, expiration)
                         VALUES ($1, $2, $3);`,
                        [user.account_id, authToken, expiration]
                    );
                    if (user.name != data.username) {
                        console.log("[GDAuth] updated username", data.username)
                        await pool.query(`UPDATE users SET name = $1 WHERE account_id = $2;`, [data.username, user.account_id]);
                    }
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

uRouter.post('/dashauth',
    body('token').notEmpty(),
    async (req: Request, res: Response) => {
        const result = validationResult(req);
        if (!result.isEmpty()) return res.status(400).json({ errors: result.array() })
        const token = req.body.token as string;
        getCache().then(pool => {
            axios.post("https://dashend.firee.dev/api/v1/verify", {token}).then(axiosRes => {
                const dashAuthData = axiosRes.data as DashAuth;
                const data = dashAuthData.data;
                if (!process.env.PRODUCTION) {
                    console.log("[DashAuth]", data);
                }
                if (axiosRes.data == "-1") return res.status(403).json({error: "Invalid token."});
                pool.query("INSERT INTO users (auth_method, account_id, name) VALUES ($1, $2, $3) ON CONFLICT (account_id) DO NOTHING RETURNING *;", ["dashauth", data.id, data.username]).then(async userResult => {
                    let user = userResult.rows[0];
                    if (!user) {
                        const existingUserResult = await pool.query(
                            `SELECT * FROM users WHERE account_id = $1;`,
                            [data.id]
                        );
                        user = existingUserResult.rows[0];
                    }
                    if (!user) return res.status(500).json({ error: "Unable to retrieve or create user." });
                    const authToken = generateAuthToken(user.account_id);
                    const expiration = new Date(Date.now() + ((60 * 60 * 1000) * 24) * 14); // 2 weeks
                    await pool.query(
                        `INSERT INTO user_tokens (account_id, token, expiration)
                         VALUES ($1, $2, $3);`,
                        [user.account_id, authToken, expiration]
                    );
                    if (user.name != data.username) {
                        console.log("[DashAuth] updated username", data.username)
                        await pool.query(`UPDATE users SET name = $1 WHERE account_id = $2;`, [data.username, user.account_id]);
                    }
                    res.status(200).json({ token: authToken });
                }).catch(err => {
                    console.error(err);
                    res.status(500).json({ error: "Something went wrong when trying to create a user." })
                })

            }).catch(e => {
                if (e.status != 401) {
                    console.error(e);
                    res.status(500).send({error: "Something went wrong when trying to communicate with DashEnd servers."})
                } else {
                    console.log("TOKEN NOT RETRIEVED")
                    res.status(400).json({error: "Token was not retrieved in time. Try again."});
                }
            })
        }).catch(e => {
            console.error(e);
            res.sendStatus(500);
        });
    }
);

uRouter.post('/argon',
    body('account_id').notEmpty().isNumeric(),
    body('token').notEmpty(),
    body('username').notEmpty(),
    async (req: Request, res: Response) => {
        const result = validationResult(req);
        if (!result.isEmpty()) return res.status(400).json({ errors: result.array() })
        const token = req.body.token as string;
        const usernameCheck = req.body.username as string;
        const accountID = parseInt(req.body.account_id as string);
        getCache().then(pool => {
            axios(`https://argon.globed.dev/v1/validation/check_strong?account_id=${accountID}&username=${encodeURIComponent(usernameCheck)}&authtoken=${encodeURIComponent(token)}`, {
                headers: {
                    'User-Agent': 'ow-server/1.0.0'
                }
            }).then(async axiosRes => {
                const data = axiosRes.data as Argon;
                if (!process.env.PRODUCTION) {
                    console.log("[Argon]", data);
                }
                if (axiosRes.data == "-1") return res.status(403).json({error: "Invalid token."});
                if (data.cause) return res.status(401).json({error: `Invalid token: ${data.cause}`});
                if (!data.valid && !data.valid_weak) return res.status(403).json({error: "Invalid token."});
                const existingUserResult0 = await pool.query(
                    `SELECT name, account_id FROM users WHERE account_id = $1;`,
                    [accountID]
                );
                if (existingUserResult0.rows.length == 1) {
                    const user = existingUserResult0.rows[0];
                    if (user.name != data.username && data.valid) {
                        await pool.query(`UPDATE users SET name = $1 WHERE account_id = $2;`, [data.username, user.account_id]);
                    }
                } else if (!data.valid) {
                    return res.status(403).json({error: "Username not valid. Refresh your GD login."});
                }
                pool.query("INSERT INTO users (auth_method, account_id, name) VALUES ($1, $2, $3) ON CONFLICT (account_id) DO NOTHING RETURNING *;", ["argon", accountID, data.username]).then(async userResult => {
                    let user = userResult.rows[0];
                    if (!user) {
                        const existingUserResult = await pool.query(
                            `SELECT * FROM users WHERE account_id = $1;`,
                            [accountID]
                        );
                        user = existingUserResult.rows[0];
                    }
                    if (!user) return res.status(500).json({ error: "Unable to retrieve or create user." });
                    const authToken = generateAuthToken(user.account_id);
                    const expiration = new Date(Date.now() + ((60 * 60 * 1000) * 24) * 14); // 2 weeks
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
                if (e.status != 401) {
                    console.error(e);
                    res.status(500).send({error: "Something went wrong when trying to communicate with DashEnd servers."})
                } else {
                    console.log("TOKEN NOT RETRIEVED")
                    res.status(400).json({error: "Token was not retrieved in time. Try again."});
                }
            })
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
