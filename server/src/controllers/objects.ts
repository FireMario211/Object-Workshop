// please do not cry of this code

import ObjectData from '@/Components/Object';
import { getCache } from '../postgres';
import { Router, Request, Response } from 'express'
import { query, body, param, validationResult, CustomValidator } from 'express-validator';
import { verifyToken, banCheck } from './user';
import axios from 'axios';
import moment from 'moment'
import { UserData } from '@/Components/User';
import { cacheMiddleware, CacheManager } from '../cache';

const allowedTags = ["Font", "Decoration", "Gameplay", "Art", "Structure", "Custom", "Icon", "Meme", "Technical", "Particles", "Triggers", "SFX", "Effects", "Auto Build", "Recreation"];

const oRouter = Router();

const isAscii: CustomValidator = (value: string) => {
    return /^[\x00-\x7F]*$/.test(value); // Checks if all characters are within the ASCII range
};

const areValidTags: CustomValidator = (tags: string[]) => {
    return tags.every(tag => allowedTags.includes(tag));
};

// call this route and cache it in ObjectWorkshop
oRouter.get('/objects/tags', async (_: Request, res: Response) => {
    return res.json(allowedTags);
})

function convertRowToObject(row: any): ObjectData {
    return {
        id: row.id,
        account_id: row.account_id,
        account_name: row.account_name,
        created: moment(row.timestamp).fromNow(),
        updated: moment(row.updated_at).fromNow(),
        name: row.name,
        description: row.description,
        downloads: row.downloads,
        favorites: row.favorites,
        rating_count: parseInt(row.rating_count.toString()),
        rating: parseFloat(row.rating.toString()),
        tags: row.tags,
        featured: row.featured,
        status: row.status,
        version: row.version,
        data: row.data
    };
}

enum ReviewStatus {
    Pending = 0,
    Updated = 1,
    Approved = 2,
    Rejected = 3
}

interface WebhookObjData {
    account_name: string;
    name: string;
    id: number;
    description: string;
    tags: Array<string>;
    version: number;
    data: string;
};

function sendWebhook(object: WebhookObjData, status: ReviewStatus, reviewer?: string) {
    const embeds = {
        "content": null,
        "embeds": [
            {
                "title": ["Object Uploaded!", `Object Updated! (Version ${object.version})`, "Object Approved!", "Object Rejected!"][status],
                "color": [0x00AAFF, 0x00AAFF, 0x00FF00, 0xFF0000][status],
                "fields": [
                    {
                        "name": "Name",
                        "value": object.name,
                        "inline": true
                    },
                    {
                        "name": "Author",
                        "value": object.account_name,
                        "inline": true
                    },
                    {
                        "name": "Object ID",
                        "value": object.id,
                        "inline": true
                    },
                    {
                        "name": "Description",
                        "value": object.description
                    },
                    {
                        "name": "Objects",
                        "value": object.data.split(";").length,
                        "inline": true
                    },
                    {
                        "name": "Tags",
                        "value": object.tags.join(", "),
                        "inline": true
                    },
                ],
                "timestamp": new Date()
            }
        ],
        "attachments": []
    }
    if (reviewer) {
        embeds.embeds[0].fields.push({
            "name": (status == ReviewStatus.Approved) ? "Approved by" : "Rejected by",
            "value": reviewer
        })
    }
    const webhookURL = [process.env.DISCORD_PENDING_WEBHOOK,process.env.DISCORD_PENDING_WEBHOOK,process.env.DISCORD_APPROVE_WEBHOOK,process.env.DISCORD_REJECT_WEBHOOK][status]
    axios({url: webhookURL, method: "POST", headers: {"Content-Type": "application/json"}, data: embeds}).then((response) => {
        console.log("Webhook delivered successfully");
        return response;
    }).catch((error) => {
        console.log(error);
        return error;
    });
}

function convertRowsToObjects(rows: any): Array<ObjectData> {
    return rows.map(convertRowToObject);
}

const blacklistedObjectIDs = [142, 3800];

oRouter.post('/objects/upload',
    body('token').notEmpty().isString(),
    body('name').notEmpty().isString(),
    body('description').notEmpty().isString(),
    body('tags').isArray().custom(areValidTags).withMessage(`Tags must be one of: ${allowedTags.join(', ')}`),
    body('tags.*').isString(),
    body('data').notEmpty().isString().custom(isAscii),
    async (req: Request, res: Response) => {
        // name should be 64 max 
        // description should be 300 max
        const result = validationResult(req);
        if (!result.isEmpty()) return res.status(400).json({ errors: result.array() })
        const { token, name, description, data } = req.body;
        let tags = req.body.tags as Array<string>;
        if (name.length > 64) return res.status(413).json({error: "The name cannot be more than 64 characters long!"});
        if (description.length > 500) return res.status(413).json({error: "The description cannot be more than 500 characters long!"});
        const splitData: Array<string> = data.split(";");
        if (splitData.length > 50000) return res.status(413).json({error: "You cannot upload a custom object with more than 50,000 objects!"})
        const hasBlacklistedIDs = splitData.find(objStr => {
            //`1,3993,2,-105,3,105,128,13.066,129,13.066;`
            const splitObj = objStr.split(',');
            if (splitObj.length == 1) return true;
            if (blacklistedObjectIDs.includes(parseInt(splitObj[1]))) return true;
            return false;
        })
        if (hasBlacklistedIDs) return res.status(403).json({error: "Blacklisted IDs are not allowed."});
        if (!tags || !tags.length) return res.status(400).json({error: "You need to add at least 1 tag!"});
        if (tags.length > 5) return res.status(413).json({error: "You can only add a maximum of 5 tags!"});
        if (data.length == 1) return res.status(400).json({error: "hi my name is firee"});
        if (data.length == 2) return res.status(400).json({error: "hola me llamo firee"});
        if (data.length == 3) return res.status(400).json({error: "salut jm'appelle firee"});
        if (data.length == 4) return res.status(400).json({error: "...what are"});
        if (data.length == 5) return res.status(400).json({error: "you doing!?"});
        getCache().then(pool => { // returns a PoolClient
            verifyToken(pool, token).then(async verifyRes => {
                if (!verifyRes.valid && verifyRes.expired) {
                    return res.status(410).json({ error: verifyRes.message });
                } else if (!verifyRes.valid) {
                    return res.status(401).json({ error: verifyRes.message });
                }
                if (!verifyRes.user) return res.status(404).json({error: "Couldn't retrieve user."});
                if (banCheck(res, verifyRes.user, 1)) return;
                try {
                    const dupCheck = await pool.query("SELECT id FROM objects WHERE data = $1 LIMIT 1;", [data]);
                    if (dupCheck.rowCount != null && dupCheck.rowCount > 0) return res.status(409).json({error: "You cannot upload an object that already exists!"});
                    const uploadedCheck = await pool.query("SELECT COUNT(*) as count FROM objects WHERE account_id = $1 AND status = 0;", [verifyRes.user.account_id]);
                    if (uploadedCheck.rows[0].count >= 10) return res.status(413).json({error: "Please wait until at least 10 of your objects have been reviewed!"});
                    const insertQuery = `
                        INSERT INTO objects (account_id, name, description, tags, status, data)
                        VALUES ($1, $2, $3, $4, $5, $6)
                        RETURNING id, timestamp;
                    `;
                    const insertResult = await pool.query(insertQuery, [verifyRes.user.account_id, name, description, tags, (verifyRes.user.role >= 1) ? 1 : 0, data]);
                    if (insertResult.rowCount === 1) {
                        const object = insertResult.rows[0];
                        res.status(200).json({
                            id: object.id,
                            name,
                            description,
                            tags,
                            timestamp: object.timestamp
                        });
                        sendWebhook({
                            id: object.id,
                            name,
                            description,
                            tags,
                            account_name: verifyRes.user.name,
                            data,
                            version: 1
                        }, ReviewStatus.Pending);
                    } else {
                        res.status(500).json({ error: "Failed to upload object." });
                    }
                } catch (e) {
                    console.error(e)
                    res.status(500).json({error: "Something went wrong when uploading."})
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


oRouter.post('/objects/:id/overwrite',
    param('id').isInt({min: 0, max: 2147483647}).notEmpty(),
    body('token').notEmpty().isString(),
    body('data').notEmpty().isString().custom(isAscii),
    async (req: Request, res: Response) => {
        const result = validationResult(req);
        if (!result.isEmpty()) return res.status(400).json({ errors: result.array() })
        const objectID = req.params.id;
        const { token, data } = req.body;
        const splitData: Array<string> = data.split(";");
        if (splitData.length > 50000) return res.status(413).json({error: "You cannot upload a custom object with more than 50,000 objects!"})
        const hasBlacklistedIDs = splitData.find(objStr => {
            //`1,3993,2,-105,3,105,128,13.066,129,13.066;`
            const splitObj = objStr.split(',');
            if (splitObj.length == 1) return true;
            if (blacklistedObjectIDs.includes(parseInt(splitObj[1]))) return true;
            return false;
        })
        if (hasBlacklistedIDs) return res.status(403).json({error: "Blacklisted IDs are not allowed."});
        if (data.length == 1) return res.status(400).json({error: "hi my name is firee"});
        if (data.length == 2) return res.status(400).json({error: "hola me llamo firee"});
        if (data.length == 3) return res.status(400).json({error: "salut jm'appelle firee"});
        if (data.length == 4) return res.status(400).json({error: "...what are"});
        if (data.length == 5) return res.status(400).json({error: "you doing!?"});
        getCache().then(pool => { // returns a PoolClient
            verifyToken(pool, token).then(async verifyRes => {
                if (!verifyRes.valid && verifyRes.expired) {
                    return res.status(410).json({ error: verifyRes.message });
                } else if (!verifyRes.valid) {
                    return res.status(401).json({ error: verifyRes.message });
                }
                if (!verifyRes.user) return res.status(404).json({error: "Couldn't retrieve user."});
                if (banCheck(res, verifyRes.user, 1)) return;
                try {
                    const query = await pool.query("SELECT * FROM objects WHERE id = $1 AND status != 3", [objectID])
                    if (!query.rows.length) return res.status(404).json({error: "Object not found."});
                    const objData: ObjectData = query.rows[0];
                    if (objData.account_id != verifyRes.user.account_id) return res.status(403).json({error: "This is not your object!"});
                    if (verifyRes.user.role == 0 && !objData.featured) {
                        await pool.query("UPDATE objects SET data = $1, status = 0, updated_at = $2, version = version + 1 WHERE id = $3", [data, new Date(), objectID]);
                    } else {
                        await pool.query("UPDATE objects SET data = $1, updated_at = $2, version = version + 1 WHERE id = $3", [data, new Date(), objectID]);
                    }
                    res.status(200).json({ message: "Object updated!" });
                    sendWebhook({
                        id: objData.id,
                        name: objData.name,
                        description: objData.description,
                        tags: objData.tags,
                        account_name: verifyRes.user.name,
                        data,
                        version: objData.version + 1
                    }, ReviewStatus.Updated);
                    CacheManager.deletePattern(`GET:/user/${objData.account_id}`);
                    CacheManager.deletePattern(`GET:/objects/${objectID}`);
                } catch (e) {
                    console.error(e)
                    res.status(500).json({error: "Something went wrong when uploading."})
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

oRouter.get('/objects/:id', param("id").isInt({min: 0, max: 2147483647}).notEmpty(), cacheMiddleware(120, (req) => {
    const category = parseInt(req.query.category as string) || 0;
    return !req.query["no-cache"] || category != 4;
}), (req: Request, res: Response) => {
    const result = validationResult(req);
    if (!result.isEmpty()) return res.status(400).json({ errors: result.array() })
    const objectID = req.params.id;
    getCache().then(async pool => {
        try {
            let query = `
                SELECT
                    o.*,
                    u.name as account_name,
                    COALESCE(AVG(orate.stars), 0) as rating,
                    COUNT(orate.stars) as rating_count
                FROM
                    objects o
                LEFT JOIN
                    ratings orate ON o.id = orate.object_id
                JOIN
                    users u ON o.account_id = u.account_id
                WHERE o.id = $1 AND o.status = 1
                GROUP BY o.id, u.name
            `;
            const result = await pool.query(query, [objectID]);
            if (result.rows.length == 0) return res.status(404).json({error: "Object not found."});
            const row: ObjectData = result.rows[0];
            res.json(convertRowToObject(row));
        } catch (e) {
            console.error(e);
            res.status(500).json({ error: 'Internal server error' });
        }
    }).catch(e => {
        console.error(e);
        res.status(500).json({ error: 'Internal server error' });
    });
})

oRouter.post('/objects/:id/rate',
    param('id').isInt({min: 0, max: 2147483647}).notEmpty(),
    body('token').notEmpty().isString(),
    body('stars').notEmpty().isInt({min: 1, max: 5}),
    (req: Request, res: Response) => {
        const result = validationResult(req);
        if (!result.isEmpty()) return res.status(400).json({ errors: result.array() })
        const token = req.body.token as string;
        let stars = parseInt(req.body.stars as string);
        if (stars > 5) stars = 5;
        if (stars < 1) stars = 1;
        // why would this ever happen
        const objectID = req.params.id;
        getCache().then(pool => {
            verifyToken(pool, token).then(async verifyRes => {
                if (!verifyRes.valid && verifyRes.expired) {
                    return res.status(410).json({ error: verifyRes.message });
                } else if (!verifyRes.valid) {
                    return res.status(401).json({ error: verifyRes.message });
                }
                const accountID = verifyRes.user?.account_id;
                if (!verifyRes.user) return;
                if (banCheck(res, verifyRes.user, 3)) return;
                try {
                    const objExists = await pool.query("SELECT EXISTS (SELECT 1 FROM objects WHERE id = $1 AND status = 1)", [objectID])
                    if (!objExists.rows[0].exists) return res.status(404).json({error: "Object not found."}); 
                    const query = `
                        INSERT INTO ratings (object_id, account_id, stars)
                        VALUES ($1, $2, $3)
                        ON CONFLICT (object_id, account_id)
                        DO UPDATE SET stars = EXCLUDED.stars, timestamp = CURRENT_TIMESTAMP
                    `;
                    await pool.query(query, [objectID, accountID, stars]);
                    res.status(200).json({ message: `Sent rating of ${stars} stars!`});
                    CacheManager.delete(`GET:/objects/${objectID}`);
                } catch (e) {
                    console.error(e);
                    res.status(500).json({ error: 'Internal server error' });
                }
            }).catch(() => {
                res.status(500).json({ error: 'Internal server error' });
            })
        }).catch(e => {
            console.error(e);
            res.status(500).json({ error: 'Internal server error' });
        });
    }
)

oRouter.post('/objects/:id/comment',
    param('id').isInt({min: 0, max: 2147483647}).notEmpty(),
    body('token').notEmpty().isString(),
    body('data').notEmpty().isString(),
    (req: Request, res: Response) => {
        const result = validationResult(req);
        if (!result.isEmpty()) return res.status(400).json({ errors: result.array() })
        const token = req.body.token as string;
        const data = req.body.data as string;
        if (data.length > 100) return res.status(413).json({error: "The comment cannot be more than 100 characters long!"});
        const objectID = req.params.id;
        getCache().then(pool => {
            verifyToken(pool, token).then(async verifyRes => {
                if (!verifyRes.valid && verifyRes.expired) {
                    return res.status(410).json({ error: verifyRes.message });
                } else if (!verifyRes.valid) {
                    return res.status(401).json({ error: verifyRes.message });
                }
                if (!verifyRes.user) return;
                const accountID = verifyRes.user.account_id;
                if (banCheck(res, verifyRes.user, 2)) return;
                try {
                    const objExists = await pool.query("SELECT EXISTS (SELECT 1 FROM objects WHERE id = $1 AND status = 1)", [objectID])
                    if (!objExists.rows[0].exists) return res.status(404).json({error: "Object not found."}); 
                    await pool.query("INSERT INTO comments (object_id, account_id, content) VALUES ($1, $2, $3)", [objectID, accountID, data]);
                    res.status(200).json({ message: `Sent!`});
                    CacheManager.delete(`GET:/objects/${objectID}/comments`);
                } catch (e) {
                    console.error(e);
                    res.status(500).json({ error: 'Internal server error' });
                }
            }).catch(() => {
                res.status(500).json({ error: 'Internal server error' });
            })
        }).catch(e => {
            console.error(e);
            res.status(500).json({ error: 'Internal server error' });
        });
    }
)

oRouter.post('/objects/:id/comments/:commentid/pin',
    param('id').isInt({min: 0, max: 2147483647}).notEmpty(),
    body('token').notEmpty().isString(),
    param('commentid').notEmpty().isInt({min: 0, max: 2147483647}),
    (req: Request, res: Response) => {
        const result = validationResult(req);
        if (!result.isEmpty()) return res.status(400).json({ errors: result.array() })
        const token = req.body.token as string;
        const commentID = parseInt(req.params.commentid);
        const objectID = req.params.id;
        getCache().then(pool => {
            verifyToken(pool, token).then(async verifyRes => {
                if (!verifyRes.valid && verifyRes.expired) {
                    return res.status(410).json({ error: verifyRes.message });
                } else if (!verifyRes.valid) {
                    return res.status(401).json({ error: verifyRes.message });
                }
                if (!verifyRes.user) return;
                const accountID = verifyRes.user.account_id;
                if (banCheck(res, verifyRes.user, 3)) return;
                try {
                    if (verifyRes.user && verifyRes.user.role == 3) {
                        const objExists = await pool.query("SELECT EXISTS (SELECT 1 FROM objects WHERE id = $1)", [objectID])
                        if (!objExists.rows[0].exists) return res.status(404).json({error: "Object not found."}); 
                    } else {
                        const objExists = await pool.query("SELECT EXISTS (SELECT 1 FROM objects WHERE id = $1 AND account_id = $2)", [objectID, accountID])
                        if (!objExists.rows[0].exists) return res.status(404).json({error: "Object not found."}); 
                    } 
                    await pool.query("UPDATE comments SET pinned = FALSE WHERE object_id = $1", [objectID]);
                    await pool.query("UPDATE comments SET pinned = TRUE WHERE id = $1", [commentID]);
                    CacheManager.delete(`GET:/objects/${objectID}/comments`);
                    res.status(200).json({ message: `Pinned!`});
                } catch (e) {
                    console.error(e);
                    res.status(500).json({ error: 'Internal server error' });
                }
            }).catch(() => {
                res.status(500).json({ error: 'Internal server error' });
            })
        }).catch(e => {
            console.error(e);
            res.status(500).json({ error: 'Internal server error' });
        });
    }
)

oRouter.post('/objects/:id/comments/:commentid/vote',
    param('id').isInt({min: 0, max: 2147483647}).notEmpty(),
    body('token').notEmpty().isString(),
    body('like').notEmpty().isInt({min: 0, max: 1}),
    (req: Request, res: Response) => {
        const result = validationResult(req);
        if (!result.isEmpty()) return res.status(400).json({ errors: result.array() })
        const token = req.body.token as string;
        const like = parseInt(req.body.like as string);
        const commentID = parseInt(req.params.commentid);
        const objectID = req.params.id;
        getCache().then(pool => {
            verifyToken(pool, token).then(async verifyRes => {
                if (!verifyRes.valid && verifyRes.expired) {
                    return res.status(410).json({ error: verifyRes.message });
                } else if (!verifyRes.valid) {
                    return res.status(401).json({ error: verifyRes.message });
                }
                const accountID = verifyRes.user?.account_id;
                if (!verifyRes.user) return res.status(403).json({error: "...waht"});
                try {
                    const objExists = await pool.query("SELECT EXISTS (SELECT 1 FROM objects WHERE id = $1 AND status = 1)", [objectID])
                    if (!objExists.rows[0].exists) return res.status(404).json({error: "Object not found."}); 
                    const commentExists = await pool.query("SELECT EXISTS (SELECT 1 FROM comments WHERE id = $1)", [commentID])
                    if (!commentExists.rows[0].exists) return res.status(404).json({error: "Comment not found."}); 
                    if (verifyRes.user.c_likes.includes(commentID) || verifyRes.user.c_dislikes.includes(commentID)) {
                        if ((verifyRes.user.c_likes.includes(commentID) && like == 1) || (verifyRes.user.c_dislikes.includes(commentID) && like == 0)) return res.status(400).json({ error: "You already voted this comment." });
                        await pool.query("UPDATE users SET c_likes = ARRAY_REMOVE(c_likes, $1) WHERE account_id = $2", [commentID, accountID]);
                        await pool.query("UPDATE users SET c_dislikes = ARRAY_REMOVE(c_dislikes, $1) WHERE account_id = $2", [commentID, accountID]);
                        if (like == 0) { // dislike
                            await pool.query("UPDATE users SET c_dislikes = ARRAY_APPEND(c_dislikes, $1) WHERE account_id = $2", [commentID, accountID]);
                            await pool.query("UPDATE comments SET likes = likes - 2 WHERE id = $1", [commentID]);
                        } else { // like
                            await pool.query("UPDATE users SET c_likes = ARRAY_APPEND(c_likes, $1) WHERE account_id = $2", [commentID, accountID]);
                            await pool.query("UPDATE comments SET likes = likes + 2 WHERE id = $1", [commentID]);
                        }
                        CacheManager.delete(`GET:/objects/${objectID}/comments`);
                        res.status(200).json({ message: "Voted!" });
                    } else {
                        if (like == 0) { // dislike
                            await pool.query("UPDATE users SET c_dislikes = ARRAY_APPEND(c_dislikes, $1) WHERE account_id = $2", [commentID, accountID]);
                            await pool.query("UPDATE comments SET likes = likes - 1 WHERE id = $1", [commentID]);
                        } else { // like
                            await pool.query("UPDATE users SET c_likes = ARRAY_APPEND(c_likes, $1) WHERE account_id = $2", [commentID, accountID]);
                            await pool.query("UPDATE comments SET likes = likes + 1 WHERE id = $1", [commentID]);
                        }
                        CacheManager.delete(`GET:/objects/${objectID}/comments`);
                        res.status(200).json({ message: "Voted!" });
                    }
                } catch (e) {
                    console.error(e);
                    res.status(500).json({ error: 'Internal server error' });
                }
            }).catch(() => {
                res.status(500).json({ error: 'Internal server error' });
            })
        }).catch(e => {
            console.error(e);
            res.status(500).json({ error: 'Internal server error' });
        });
    }
)

oRouter.post('/objects/:id/comments/:commentid/delete',
    param('id').isInt({min: 0, max: 2147483647}).notEmpty(),
    body('token').notEmpty().isString(),
    param('commentid').notEmpty().isInt({min: 0, max: 2147483647}),
    (req: Request, res: Response) => {
        const result = validationResult(req);
        if (!result.isEmpty()) return res.status(400).json({ errors: result.array() })
        const token = req.body.token as string;
        const commentID = parseInt(req.params.commentid);
        const objectID = req.params.id;
        getCache().then(pool => {
            verifyToken(pool, token).then(async verifyRes => {
                if (!verifyRes.valid && verifyRes.expired) {
                    return res.status(410).json({ error: verifyRes.message });
                } else if (!verifyRes.valid) {
                    return res.status(401).json({ error: verifyRes.message });
                }
                if (!verifyRes.user) return;
                const accountID = verifyRes.user?.account_id;
                if (banCheck(res, verifyRes.user, 3)) return;
                try {
                    const query = await pool.query("SELECT * FROM objects WHERE id = $1 AND status = 1", [objectID])
                    if (!query.rows.length) return res.status(404).json({error: "Object not found."});
                    if (verifyRes.user && (verifyRes.user.role == 3 || (query.rows[0] as ObjectData).account_id == verifyRes.user.account_id)) {
                        const commentExists = await pool.query("SELECT EXISTS (SELECT 1 FROM comments WHERE id = $1)", [commentID])
                        if (!commentExists.rows[0].exists) return res.status(404).json({error: "Comment not found."}); 
                    } else {
                        const commentExists = await pool.query("SELECT EXISTS (SELECT 1 FROM comments WHERE id = $1 AND account_id = $2)", [commentID, accountID])
                        if (!commentExists.rows[0].exists) return res.status(404).json({error: "Comment not found."}); 
                    } 
                    await pool.query("DELETE FROM comments WHERE id = $1", [commentID]);
                    CacheManager.delete(`GET:/objects/${objectID}/comments`);
                    res.status(200).json({ message: `Deleted!`});
                } catch (e) {
                    console.error(e);
                    res.status(500).json({ error: 'Internal server error' });
                }
            }).catch(() => {
                res.status(500).json({ error: 'Internal server error' });
            })
        }).catch(e => {
            console.error(e);
            res.status(500).json({ error: 'Internal server error' });
        });
    }
)

oRouter.post('/objects/:id/report',
    param('id').isInt({min: 0, max: 2147483647}).notEmpty(),
    body('reason').notEmpty().isString(),
    body('token').notEmpty().isString(),
    (req: Request, res: Response) => {
        const result = validationResult(req);
        if (!result.isEmpty()) return res.status(400).json({ errors: result.array() })
        const token = req.body.token as string;
        const reason = req.body.reason as string;
        if (reason.length > 300) return res.status(400).json({error: "Your reason cannot be more than 300 characters!"})
        // why would this ever happen
        const objectID = req.params.id;
        getCache().then(pool => {
            verifyToken(pool, token).then(async verifyRes => {
                if (!verifyRes.valid && verifyRes.expired) {
                    return res.status(410).json({ error: verifyRes.message });
                } else if (!verifyRes.valid) {
                    return res.status(401).json({ error: verifyRes.message });
                }
                if (!verifyRes.user) return;
                const accountID = verifyRes.user.account_id;
                if (banCheck(res, verifyRes.user, 1)) return;
                try {
                    const objExists = await pool.query("SELECT EXISTS (SELECT 1 FROM objects WHERE id = $1 AND status = 1)", [objectID])
                    if (!objExists.rows[0].exists) return res.status(404).json({error: "Object not found."}); 
                    const reportExists = await pool.query("SELECT EXISTS (SELECT 1 FROM reports WHERE object_id = $1 AND account_id = $2)", [objectID, accountID])
                    if (reportExists.rows[0].exists) return res.status(404).json({error: "You have already reported this object!"}); 
                    await pool.query('INSERT INTO reports (object_id, account_id, reason) VALUES ($1, $2, $3)', [objectID, accountID, reason])
                    res.status(200).json({ message: `Reported!`});
                } catch (e) {
                    console.error(e);
                    res.status(500).json({ error: 'Internal server error' });
                }
            }).catch(() => {
                res.status(500).json({ error: 'Internal server error' });
            })
        }).catch(e => {
            console.error(e);
            res.status(500).json({ error: 'Internal server error' });
        });
    }
)

oRouter.post('/objects/:id/favorite',
    param('id').isInt({min: 0, max: 2147483647}).notEmpty(),
    body('token').notEmpty().isString(),
    (req: Request, res: Response) => {
        const result = validationResult(req);
        if (!result.isEmpty()) return res.status(400).json({ errors: result.array() })
        const token = req.body.token as string;
        const objectID = req.params.id;
        getCache().then(pool => {
            verifyToken(pool, token).then(async verifyRes => {
                if (!verifyRes.valid && verifyRes.expired) {
                    return res.status(410).json({ error: verifyRes.message });
                } else if (!verifyRes.valid) {
                    return res.status(401).json({ error: verifyRes.message });
                }
                const accountID = verifyRes.user?.account_id;
                try {
                    const objExists = await pool.query("SELECT EXISTS (SELECT 1 FROM objects WHERE id = $1 AND status = 1)", [objectID])
                    if (!objExists.rows[0].exists) return res.status(404).json({error: "Object not found."}); 
                    let query = "";
                    
                    let message = ""
                    if (verifyRes.user?.favorites.includes(parseInt(objectID))) {
                        query = "UPDATE users SET favorites = ARRAY_REMOVE(favorites, $1) WHERE account_id = $2";
                        message = "Unfavorited object!";
                    } else {
                        query = "UPDATE users SET favorites = ARRAY_APPEND(favorites, $1) WHERE account_id = $2";
                        message = "Favorited object!";
                    }
                    await pool.query(query, [objectID, accountID]);
                    const favoriteCount = await pool.query("SELECT COUNT(*) as favorites FROM users WHERE favorites @> ARRAY[$1::integer]", [objectID]);
                    await pool.query("UPDATE objects SET favorites = $1 WHERE id = $2", [favoriteCount.rows[0].favorites, objectID]);
                    res.status(200).json({ message });
                } catch (e) {
                    console.error(e);
                    res.status(500).json({ error: 'Internal server error' });
                }
            }).catch(() => {
                res.status(500).json({ error: 'Internal server error' });
            })
        }).catch(e => {
            console.error(e);
            res.status(500).json({ error: 'Internal server error' });
        });
    }
)

oRouter.post('/objects/:id/download',
    param('id').isInt({min: 0, max: 2147483647}).notEmpty(),
    body('token').notEmpty().isString(),
    (req: Request, res: Response) => {
        const result = validationResult(req);
        if (!result.isEmpty()) return res.status(400).json({ errors: result.array() })
        const token = req.body.token as string;
        const objectID = req.params.id;
        getCache().then(pool => {
            verifyToken(pool, token).then(async verifyRes => {
                if (!verifyRes.valid && verifyRes.expired) {
                    return res.status(410).json({ error: verifyRes.message });
                } else if (!verifyRes.valid) {
                    return res.status(401).json({ error: verifyRes.message });
                }
                const accountID = verifyRes.user?.account_id;
                try {
                    const objExists = await pool.query("SELECT EXISTS (SELECT 1 FROM objects WHERE id = $1 AND status = 1)", [objectID])
                    if (!objExists.rows[0].exists) return res.status(404).json({error: "Object not found."}); 
                    let message = ""
                    if (verifyRes.user?.downloaded.includes(parseInt(objectID))) {
                        message = "You already downloaded the object.";
                    } else {
                        message = "Updated download count!";
                        await pool.query("UPDATE users SET downloaded = ARRAY_APPEND(downloaded, $1) WHERE account_id = $2", [objectID, accountID]);
                        await pool.query("UPDATE objects SET downloads = downloads + 1 WHERE id = $1", [objectID]);
                    }
                    res.status(200).json({ message });
                } catch (e) {
                    console.error(e);
                    res.status(500).json({ error: 'Internal server error' });
                }
            }).catch(() => {
                res.status(500).json({ error: 'Internal server error' });
            })
        }).catch(e => {
            console.error(e);
            res.status(500).json({ error: 'Internal server error' });
        });
    }
)

oRouter.post('/objects/:id/delete',
    param('id').isInt({min: 0, max: 2147483647}).notEmpty(),
    body('token').notEmpty().isString(),
    (req: Request, res: Response) => {
        const result = validationResult(req);
        if (!result.isEmpty()) return res.status(400).json({ errors: result.array() })
        const token = req.body.token as string;
        const objectID = req.params.id;
        getCache().then(pool => {
            verifyToken(pool, token).then(async verifyRes => {
                if (!verifyRes.valid && verifyRes.expired) {
                    return res.status(410).json({ error: verifyRes.message });
                } else if (!verifyRes.valid) {
                    return res.status(401).json({ error: verifyRes.message });
                }
                const accountID = verifyRes.user?.account_id;
                try {
                    if (verifyRes.user && verifyRes.user.role == 3) {
                        const objExists = await pool.query("SELECT account_id FROM objects WHERE id = $1", [objectID])
                        if (!objExists.rows.length) return res.status(404).json({error: "Object not found."});
                        CacheManager.deletePattern(`GET:/user/${objExists.rows[0].account_id}`);
                    } else {
                        const objExists = await pool.query("SELECT EXISTS (SELECT 1 FROM objects WHERE id = $1 AND account_id = $2)", [objectID, accountID])
                        if (!objExists.rows[0].exists) return res.status(404).json({error: "Object not found."}); 
                        CacheManager.deletePattern(`GET:/user/${accountID}`);
                    }
                    await pool.query("DELETE FROM objects WHERE id = $1", [objectID]);
                    res.status(200).json({ message: "Deleted object!" });
                    CacheManager.delete(`GET:/objects/${objectID}`);
                    CacheManager.deletePattern(`GET:/objects`);
                } catch (e) {
                    console.error(e);
                    res.status(500).json({ error: 'Internal server error' });
                }
            }).catch(() => {
                res.status(500).json({ error: 'Internal server error' });
            })
        }).catch(e => {
            console.error(e);
            res.status(500).json({ error: 'Internal server error' });
        });
    }
)

oRouter.post('/objects/:id/update',
    param('id').isInt({min: 0, max: 2147483647}).notEmpty(),
    body('token').notEmpty().isString(),
    body('name').notEmpty().isString(),
    body('description').notEmpty().isString(),
    body('tags').isArray().custom(areValidTags).withMessage(`Tags must be one of: ${allowedTags.join(', ')}`),
    body('tags.*').isString(),
    async (req: Request, res: Response) => {
        // name should be 64 max 
        // description should be 300 max
        const result = validationResult(req);
        if (!result.isEmpty()) return res.status(400).json({ errors: result.array() })
        const objectID = req.params.id;
        const { token, name, description } = req.body;
        let tags = req.body.tags as Array<string>;
        if (name.length > 64) return res.status(413).json({error: "The name cannot be more than 64 characters long!"});
        if (description.length > 500) return res.status(413).json({error: "The description cannot be more than 500 characters long!"});
        if (!tags || !tags.length) tags = [];
        if (tags.length > 5) return res.status(413).json({error: "You can only add a maximum of 5 tags!"});
        getCache().then(pool => { // returns a PoolClient
            verifyToken(pool, token).then(async verifyRes => {
                if (!verifyRes.valid && verifyRes.expired) {
                    return res.status(410).json({ error: verifyRes.message });
                } else if (!verifyRes.valid) {
                    return res.status(401).json({ error: verifyRes.message });
                }
                if (!verifyRes.user) return res.status(404).json({error: "Couldn't retrieve user."});
                const accountID = verifyRes.user.account_id;
                try {
                    let query = `
                        UPDATE objects SET
                        name = $1,
                        description = $2,
                        tags = $3 WHERE id = $4
                    `;
                    if (verifyRes.user && verifyRes.user.role >= 2) {
                        const objExists = await pool.query("SELECT EXISTS (SELECT 1 FROM objects WHERE id = $1)", [objectID])
                        if (!objExists.rows[0].exists) return res.status(404).json({error: "Object not found."}); 
                        if (verifyRes.user.role == 3) {
                            await pool.query(query, [name, description, tags, objectID]);
                        } else {
                            query = `
                                UPDATE objects SET
                                tags = $1 WHERE id = $2 AND status = 0
                            `;
                            await pool.query(query, [tags, objectID]);
                        }
                    } else {
                        const objExists = await pool.query("SELECT EXISTS (SELECT 1 FROM objects WHERE id = $1 AND account_id = $2)", [objectID, accountID])
                        if (!objExists.rows[0].exists) return res.status(404).json({error: "Object not found."}); 
                        await pool.query(query, [name, description, tags, objectID]);
                        CacheManager.deletePattern(`GET:/user/${accountID}`);
                    }
                    CacheManager.delete(`GET:/objects/${objectID}`);
                    return res.status(200).json({message: "Updated object!"});
                } catch (e) {
                    console.error(e)
                    res.status(500).json({error: "Something went wrong when uploading."})
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

oRouter.post('/objects/:id/accept',
    param('id').isInt({min: 0, max: 2147483647}).notEmpty(),
    body('token').notEmpty().isString(),
    (req: Request, res: Response) => {
        const result = validationResult(req);
        if (!result.isEmpty()) return res.status(400).json({ errors: result.array() })
        const token = req.body.token as string;
        const objectID = req.params.id;
        getCache().then(pool => {
            verifyToken(pool, token).then(async verifyRes => {
                if (!verifyRes.valid && verifyRes.expired) {
                    return res.status(410).json({ error: verifyRes.message });
                } else if (!verifyRes.valid) {
                    return res.status(401).json({ error: verifyRes.message });
                }
                if (!verifyRes.user) return res.status(403).json({error: "...what"});
                if (verifyRes.user && verifyRes.user.role < 2) return res.status(403).json({ error: "No permission" });

                try {
                    const query = await pool.query("SELECT * FROM objects WHERE id = $1 AND status = 0", [objectID])
                    if (!query.rows.length) return res.status(404).json({error: "Object not found."});
                    const objData: ObjectData = query.rows[0];
                    const query2 = await pool.query("SELECT name FROM users WHERE account_id = $1", [objData.account_id]);
                    if (query2.rows.length) {
                        objData.account_name = query2.rows[0].name;
                    }
                    if (verifyRes.user.name == process.env.DV) {
                        verifyRes.user.name = process.env.RV as string
                    }
                    await pool.query("UPDATE objects SET status = 1 WHERE id = $1", [objectID]);
                    sendWebhook({
                        id: objData.id,
                        name: objData.name,
                        description: objData.description,
                        tags: objData.tags,
                        account_name: objData.account_name,
                        data: objData.data,
                        version: objData.version
                    }, ReviewStatus.Approved, verifyRes.user.name);
                    res.status(200).json({ message: "Accepted!" });
                    CacheManager.deletePattern(`GET:/user/${objData.account_id}`);
                } catch (e) {
                    console.error(e);
                    res.status(500).json({ error: 'Internal server error' });
                }
            }).catch(() => {
                res.status(500).json({ error: 'Internal server error' });
            })
        }).catch(e => {
            console.error(e);
            res.status(500).json({ error: 'Internal server error' });
        });
    }
)

oRouter.post('/objects/:id/reject',
    param('id').isInt({min: 0, max: 2147483647}).notEmpty(),
    body('token').notEmpty().isString(),
    (req: Request, res: Response) => {
        const result = validationResult(req);
        if (!result.isEmpty()) return res.status(400).json({ errors: result.array() })
        const token = req.body.token as string;
        const objectID = req.params.id;
        getCache().then(pool => {
            verifyToken(pool, token).then(async verifyRes => {
                if (!verifyRes.valid && verifyRes.expired) {
                    return res.status(410).json({ error: verifyRes.message });
                } else if (!verifyRes.valid) {
                    return res.status(401).json({ error: verifyRes.message });
                }
                if (!verifyRes.user) return res.status(403).json({error: "...what"});
                if (verifyRes.user && verifyRes.user.role < 2) return res.status(403).json({ error: "No permission" });
                try {
                    const query = await pool.query("SELECT * FROM objects WHERE id = $1 AND status = 0", [objectID])
                    if (!query.rows.length) return res.status(404).json({error: "Object not found."});
                    const objData: ObjectData = query.rows[0];
                    const query2 = await pool.query("SELECT name FROM users WHERE account_id = $1", [objData.account_id]);
                    if (query2.rows.length) {
                        objData.account_name = query2.rows[0].name;
                    }
                    if (verifyRes.user.name == process.env.DV) {
                        verifyRes.user.name = process.env.RV as string
                    }
                    await pool.query("DELETE FROM objects WHERE id = $1", [objectID]);
                    res.status(200).json({ message: "Rejected!" });
                    sendWebhook({
                        id: objData.id,
                        name: objData.name,
                        description: objData.description,
                        tags: objData.tags,
                        account_name: objData.account_name,
                        data: objData.data,
                        version: objData.version
                    }, ReviewStatus.Rejected, verifyRes.user.name);
                } catch (e) {
                    console.error(e);
                    res.status(500).json({ error: 'Internal server error' });
                }
            }).catch(() => {
                res.status(500).json({ error: 'Internal server error' });
            })
        }).catch(e => {
            console.error(e);
            res.status(500).json({ error: 'Internal server error' });
        });
    }
)

oRouter.post('/objects/:id/feature',
    param('id').isInt({min: 0, max: 2147483647}).notEmpty(),
    body('token').notEmpty().isString(),
    (req: Request, res: Response) => {
        const result = validationResult(req);
        if (!result.isEmpty()) return res.status(400).json({ errors: result.array() })
        const token = req.body.token as string;
        const objectID = req.params.id;
        getCache().then(pool => {
            verifyToken(pool, token).then(async verifyRes => {
                if (!verifyRes.valid && verifyRes.expired) {
                    return res.status(410).json({ error: verifyRes.message });
                } else if (!verifyRes.valid) {
                    return res.status(401).json({ error: verifyRes.message });
                }
                if (!verifyRes.user) return res.status(403).json({error: "...what"});
                if (verifyRes.user && verifyRes.user.role < 3) return res.status(403).json({ error: "No permission" });
                try {
                    const query = await pool.query("SELECT * FROM objects WHERE id = $1", [objectID])
                    if (!query.rows.length) return res.status(404).json({error: "Object not found."});
                    const objData: ObjectData = query.rows[0];
                    const query2 = await pool.query("SELECT name FROM users WHERE account_id = $1", [objData.account_id]);
                    if (query2.rows.length) {
                        objData.account_name = query2.rows[0].name;
                    }
                    if (objData.featured == 1) {
                        await pool.query("UPDATE objects SET featured = 0 WHERE id = $1", [objectID]);
                        res.status(200).json({ message: "Unfeatured!" });
                    } else {
                        await pool.query("UPDATE objects SET featured = 1 WHERE id = $1", [objectID]);
                        res.status(200).json({ message: "Featured!" });
                    }
                    CacheManager.delete(`GET:/objects/${objectID}`);
                    CacheManager.deletePattern(`GET:/objects`);
                    CacheManager.deletePattern(`GET:/user/${objData.account_id}`);
                } catch (e) {
                    console.error(e);
                    res.status(500).json({ error: 'Internal server error' });
                }
            }).catch(() => {
                res.status(500).json({ error: 'Internal server error' });
            })
        }).catch(e => {
            console.error(e);
            res.status(500).json({ error: 'Internal server error' });
        });
    }
)

oRouter.get('/user/:id',
    param('id').isInt({min: 0, max: 2147483647}).notEmpty(),
    query('page').isInt({min: 0, max: 2147483647}).optional(),
    cacheMiddleware(60, (req) => {
        return !req.query["no-cache"];
    }),
    async (req: Request, res: Response) => {
        const result = validationResult(req);
        if (!result.isEmpty()) return res.status(400).json({ errors: result.array() })
        const userID = parseInt(req.params.id);
        getCache().then(async pool => {
            const userResult = await pool.query("SELECT account_id,name,timestamp,role,ban_reason,icon FROM users WHERE account_id = $1", [userID]);
            if (userResult.rows.length == 0) return res.status(403).json({error: "User not found."});
            const userData: UserData = userResult.rows[0];
            const page = parseInt(req.query.page as string) || 1;
            const limit = 6;
            const offset = (page - 1) * limit;
            try {
                let query = `
                    SELECT
                        o.*,
                        u.name as account_name,
                        COALESCE(AVG(orate.stars), 0) as rating,
                        COUNT(orate.stars) as rating_count,
                        COUNT(*) OVER() AS total_records
                    FROM
                        objects o
                    LEFT JOIN
                        ratings orate ON o.id = orate.object_id
                    JOIN
                        users u ON o.account_id = u.account_id
                    WHERE o.account_id = $1 AND o.status = 1
                    GROUP BY o.id, u.name
                    ORDER BY o.timestamp DESC
                    LIMIT $2 OFFSET $3
                `;
                const result = await pool.query(query, [userData.account_id, limit, offset]);
                const creatorPointsRes = await pool.query(`SELECT count(*) as points FROM objects WHERE account_id = $1 AND featured = 1`, [userData.account_id]);
                const averageRes = await pool.query(`
                    SELECT
                        COALESCE(AVG(orate.stars), 0) AS average_rating,
                        COUNT(orate.stars) AS total_rating_count
                    FROM
                        objects o
                    LEFT JOIN
                        ratings orate ON o.id = orate.object_id
                    WHERE
                        o.account_id = $1 AND o.status = 1;
                `, [userData.account_id])
                const totalRes = await pool.query(`
                    SELECT
                        SUM(downloads) AS total_downloads,
                        SUM(favorites) AS total_favorites
                    FROM
                        objects
                    WHERE
                        account_id = $1 AND status = 1;
                `, [userData.account_id])
                const totalRecords = (result.rows.length > 0) ? parseInt(result.rows[0].total_records) : 0;
                const totalPages = Math.ceil(totalRecords / limit);
                userData.uploads = totalRecords;
                userData.featured = parseInt(creatorPointsRes.rows[0].points);

                const objectData = convertRowsToObjects(result.rows)
                res.json({
                    results: objectData,
                    page,
                    total: totalRecords,
                    pageAmount: totalPages,
                    user: userData,
                    user_average: {
                        average_rating: parseFloat(averageRes.rows[0].average_rating),
                        total_rating_count: parseInt(averageRes.rows[0].total_rating_count)
                    },
                    user_total: {
                        downloads: parseInt(totalRes.rows[0].total_downloads),
                        favorites: parseInt(totalRes.rows[0].total_favorites)
                    }
                });
            } catch (e) {
                console.error(e);
                res.status(500).json({ error: 'Internal server error' });
            }
        }).catch(e => {
            console.error(e);
            res.status(500).json({ error: 'Internal server error' });
        });
    }
);

oRouter.post('/user/@me/objects',
    body('token').notEmpty().isString(),
    query('page').isInt({min: 0, max: 2147483647}).optional(),
    query('limit').isBoolean().optional(),
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
                const page = parseInt(req.query.page as string) || 1;
                const limit = (req.query.limit as string == "true") ? 2 : 6;
                const offset = (page - 1) * limit;
                try {
                    let query = `
                        SELECT
                            o.*,
                            u.name as account_name,
                            COALESCE(AVG(orate.stars), 0) as rating,
                            COUNT(orate.stars) as rating_count,
                            COUNT(*) OVER() AS total_records
                        FROM
                            objects o
                        LEFT JOIN
                            ratings orate ON o.id = orate.object_id
                        JOIN
                            users u ON o.account_id = u.account_id
                        WHERE o.account_id = $1
                        GROUP BY o.id, u.name
                        ORDER BY o.timestamp DESC
                        LIMIT $2 OFFSET $3
                    `;
                    const result = await pool.query(query, [verifyRes.user?.account_id, limit, offset]);
                    const totalRecords = (result.rows.length > 0) ? parseInt(result.rows[0].total_records) : 0;
                    const totalPages = Math.ceil(totalRecords / limit);

                    const objectData = convertRowsToObjects(result.rows)
                    res.json({
                        results: objectData,
                        page,
                        total: totalRecords,
                        pageAmount: totalPages
                    });
                } catch (e) {
                    console.error(e);
                    res.status(500).json({ error: 'Internal server error' });
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


oRouter.get('/objects/:id/comments',
    param('id').isInt({min: 0, max: 2147483647}).notEmpty(),
    query('page').isInt({min: 1, max: 2147483647}).optional(),
    query('limit').isInt({min: 1, max: 10}).optional(),
    query('filter').isInt({min: 1, max: 2}).optional(),
    cacheMiddleware(30, (req) => {
        return !req.query["no-cache"];
    }),
    async (req: Request, res: Response) => {
        const result = validationResult(req);
        if (!result.isEmpty()) return res.status(400).json({ errors: result.array() })
        const objectID = parseInt(req.params.id);
        getCache().then(async pool => {
            const page = parseInt(req.query.page as string) || 1;
            const limit = parseInt(req.query.limit as string) || 10;
            const filter = parseInt(req.query.filter as string) || 2;
            const offset = (page - 1) * limit;
            try {
                let query = `
                    SELECT
                        c.*,
                        u.name as account_name,
                        u.icon,
                        u.role,
                        COUNT(*) OVER() AS total_records
                    FROM
                        comments c
                    JOIN
                        users u ON c.account_id = u.account_id
                    WHERE c.object_id = $1
                    GROUP BY c.id, u.name, u.icon, u.role
                    ORDER BY c.pinned DESC, ${(filter == 1) ? "c.likes DESC, c.timestamp" : "c.timestamp"} DESC
                    LIMIT $2 OFFSET $3
                `;
                const result = await pool.query(query, [objectID, limit, offset]);
                const totalRecords = (result.rows.length > 0) ? parseInt(result.rows[0].total_records) : 0;
                const totalPages = Math.ceil(totalRecords / limit);

                res.json({
                    results: result.rows.map(row => {
                        row.timestamp = moment(row.timestamp).fromNow();
                        return row;
                    }),
                    page,
                    total: totalRecords,
                    pageAmount: totalPages
                });
            } catch (e) {
                console.error(e);
                res.status(500).json({ error: 'Internal server error' });
            }
        }).catch(e => {
            console.error(e);
            res.status(500).json({ error: 'Internal server error' });
        });
    }
);

oRouter.post('/objects/pending',
    body('token').notEmpty().isString(),
    query('page').isInt({min: 1, max: 2147483647}).optional(),
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
                if (verifyRes.user && verifyRes.user.role < 2) return res.status(403).json({ error: "No permission" });
                const page = parseInt(req.query.page as string) || 1;
                const limit = 9;
                const offset = (page - 1) * limit;
                try {
                    let query = `
                        SELECT
                            o.*,
                            u.name as account_name,
                            COALESCE(AVG(orate.stars), 0) as rating,
                            COUNT(orate.stars) as rating_count,
                            COUNT(*) OVER() AS total_records
                        FROM
                            objects o
                        LEFT JOIN
                            ratings orate ON o.id = orate.object_id
                        JOIN
                            users u ON o.account_id = u.account_id
                        WHERE o.status = 0
                        GROUP BY o.id, u.name
                        ORDER BY o.timestamp ASC
                        LIMIT $1 OFFSET $2
                    `;
                    const result = await pool.query(query, [limit, offset]);
                    const totalRecords = (result.rows.length > 0) ? parseInt(result.rows[0].total_records) : 0;
                    const totalPages = Math.ceil(totalRecords / limit);

                    const objectData = convertRowsToObjects(result.rows)
                    res.json({
                        results: objectData,
                        page,
                        total: totalRecords,
                        pageAmount: totalPages
                    });
                } catch (e) {
                    console.error(e);
                    res.status(500).json({ error: 'Internal server error' });
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

oRouter.post('/objects/reports',
    body('token').notEmpty().isString(),
    query('page').isInt({min: 1, max: 2147483647}).optional(),
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
                if (verifyRes.user && verifyRes.user.role < 2) return res.status(403).json({ error: "No permission" });
                const page = parseInt(req.query.page as string) || 1;
                const limit = 9;
                const offset = (page - 1) * limit;
                try {
                    let query = `
                        SELECT
                            o.*,
                            u.name as account_name,
                            COALESCE(AVG(orate.stars), 0) as rating,
                            COUNT(orate.stars) as rating_count,
                            COUNT(*) OVER() AS total_records,
                            COUNT(rep) AS report_count,
                            COALESCE(ARRAY_AGG(DISTINCT jsonb_build_object(
                                'reason', rep.reason,
                                'account_id', rep.account_id,
                                'timestamp', rep.timestamp
                            )) FILTER (WHERE rep.object_id IS NOT NULL), '{}') AS reports
                        FROM
                            objects o
                        LEFT JOIN
                            ratings orate ON o.id = orate.object_id
                        JOIN 
                            reports rep ON rep.object_id = o.id
                        JOIN
                            users u ON o.account_id = u.account_id
                        GROUP BY o.id, u.name
                        ORDER BY o.timestamp DESC
                        LIMIT $1 OFFSET $2
                    `;
                    const result = await pool.query(query, [limit, offset]);
                    const totalRecords = (result.rows.length > 0) ? parseInt(result.rows[0].total_records) : 0;
                    const totalPages = Math.ceil(totalRecords / limit);

                    const objectData = (convertRowsToObjects(result.rows) as Array<any>).map((value, index) => {
                        value.report_count = parseInt(result.rows[index].report_count);
                        value.reports = result.rows[index].reports;
                        return value;
                    });
                    res.json({
                        results: objectData,
                        page,
                        total: totalRecords,
                        pageAmount: totalPages
                    });
                } catch (e) {
                    console.error(e);
                    res.status(500).json({ error: 'Internal server error' });
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

oRouter.post('/user/@me/favorites',
    body('token').notEmpty().isString(),
    query('page').isInt({min: 1, max: 2147483647}).optional(),
    query('limit').isInt({min: 1, max: 9}).optional(),
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
                const page = parseInt(req.query.page as string) || 1;
                const limit = parseInt(req.query.limit as string) || 6;
                const offset = (page - 1) * limit;
                try {
                    let query = `
                        SELECT
                            o.*,
                            u.name as account_name,
                            COALESCE(AVG(orate.stars), 0) as rating,
                            COUNT(orate.stars) as rating_count,
                            COUNT(*) OVER() AS total_records
                        FROM
                            objects o
                        LEFT JOIN
                            ratings orate ON o.id = orate.object_id
                        JOIN
                            users u ON o.account_id = u.account_id
                        WHERE 
                            o.id = ANY((SELECT unnest(favorites) FROM users WHERE account_id = $1))
                        GROUP BY o.id, u.name
                        ORDER BY o.timestamp DESC
                        LIMIT $2 OFFSET $3
                    `;
                    const result = await pool.query(query, [verifyRes.user?.account_id, limit, offset]);
                    const totalRecords = (result.rows.length > 0) ? parseInt(result.rows[0].total_records) : 0;
                    const totalPages = Math.ceil(totalRecords / limit);

                    let objectData = convertRowsToObjects(result.rows)
                    objectData = objectData.map(obj => {
                        if (obj.status == 3) {
                            obj.data = "1,3993,2,-105,3,105,128,13.066,129,13.066;"; // GONE!
                        }
                        return obj;
                    });
                    res.json({
                        results: objectData,
                        page,
                        total: totalRecords,
                        pageAmount: totalPages
                    });
                } catch (e) {
                    console.error(e);
                    res.status(500).json({ error: 'Internal server error' });
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

oRouter.get('/objects',
    query('page').isInt({min: 1, max: 2147483647}).optional(),
    query('category').isInt({min: 0, max: 2147483647}).optional(),
    query('limit').isInt({min: 1, max: 9}).optional(),
    body('tags').optional().isString(),
    cacheMiddleware(60, (req) => {
        const category = parseInt(req.query.category as string) || 0;
        return !req.query["no-cache"] && category != 4;
    }),
    async (req: Request, res: Response) => {
        const result = validationResult(req);
        if (!result.isEmpty()) return res.status(400).json({ errors: result.array() })
        getCache().then(async pool => {
            const page = parseInt(req.query.page as string) || 1;
            const category = parseInt(req.query.category as string) || 0;
            const limit = parseInt(req.query.limit as string) || 6;

            let tags: string[] = [];
            if (req.query.tags) {
                tags = req.query.tags.toString().split(",");
            }
            if (!tags.every(tag => allowedTags.includes(tag))) return res.status(400).json({error: `Tags must be one of: ${allowedTags.join(', ')}`})

            const offset = (page - 1) * limit;
            try {
                /*let query = `
                    SELECT
                        o.*,
                        COALESCE(AVG(or.rating), 0) as rating,
                    FROM
                        objects o
                    LEFT JOIN
                        obj_ratings or ON o.id = or.object_id
                    GROUP BY
                        a.id
                    ORDER BY
                        a.timestamp DESC
                    LIMIT $1 OFFSET $2
                `;*/
                let query = ''
                if (category == 1) {
                    query += `WITH object_stats AS (`
                }
                query += `
                    SELECT
                        o.*,
                        u.name as account_name,
                        COALESCE(AVG(orate.stars), 0) as rating,
                        COUNT(orate.stars) as rating_count,
                        COUNT(*) OVER() AS total_records
                    FROM
                        objects o
                    LEFT JOIN
                        ratings orate ON o.id = orate.object_id
                    JOIN
                        users u ON o.account_id = u.account_id
                `;
                let orderBy = '';

                const queryParams: any[] = [limit, offset];

                if (tags.length > 0) {
                    query += `
                        WHERE o.tags @> $3 AND o.status = 1
                    `;
                    queryParams.push(tags);
                } else {
                    query += `
                        WHERE o.status = 1
                    `
                }

                switch (category) {
                    case 0: // Top downloads
                        orderBy = 'ORDER BY o.downloads DESC';
                        break;
                    case 1: // Most popular
                        //orderBy = 'ORDER BY rating DESC, rating_count DESC';
                        // i have a love-hate relationship with statistics
                        orderBy += `)
                        SELECT 
                            *, CASE WHEN rating_count = 0 THEN 0
                            ELSE (rating_count / (rating_count + 10)) * rating + (10 / (rating_count + 10)) * 3
                        END AS weighted_rating
                        FROM object_stats
                        ORDER BY weighted_rating DESC, rating_count DESC`
                        break;
                    case 2: // Most Liked
                        orderBy = 'ORDER BY o.favorites DESC';
                        break;
                    case 3: // Trending (based on the current week)
                        query += `
                            AND o.timestamp >= NOW() - INTERVAL '7 days'
                        `;
                        orderBy = 'ORDER BY o.downloads DESC';
                        break;
                    case 5: // Featured
                        query += `
                            AND o.featured = 1
                        `
                        orderBy = 'ORDER BY o.downloads DESC';
                        break;
                    case 4: // Most recent
                    default:
                        orderBy = 'ORDER BY o.timestamp DESC';
                        break;
                }

                query += `
                    GROUP BY o.id, u.name
                    ${orderBy}
                    LIMIT $1 OFFSET $2
                `;
                const result = await pool.query(query, queryParams);
                //const objectAmountRes = await pool.query(`SELECT count(*) AS count FROM objects ${(category == 3) ? orderBy : ""}`);
                //const objectAmount = parseInt(objectAmountRes.rows[0].count.toString());

                const totalRecords = (result.rows.length > 0) ? parseInt(result.rows[0].total_records) : 0;
                const totalPages = Math.ceil(totalRecords / limit);

                const objectData = convertRowsToObjects(result.rows)
                res.json({
                    results: objectData,
                    page,
                    total: totalRecords,
                    pageAmount: totalPages
                });
            } catch (e) {
                console.error(e);
                res.status(500).json({ error: 'Internal server error' });
            }
        }).catch(e => {
            console.error(e);
            res.status(500).json({ error: 'Internal server error' });
        });
    }
);

oRouter.post('/objects/search',
    query('query').notEmpty().isString(),
    query('page').isInt({min: 1, max: 2147483647}).optional(),
    query('limit').isInt({min: 1, max: 9}).optional(),
    body('tags').optional().isString(),
    cacheMiddleware(10, (req) => {
        return !req.query["no-cache"]
    }),
    async (req: Request, res: Response) => {
        const result = validationResult(req);
        if (!result.isEmpty()) return res.status(400).json({ errors: result.array() })
        getCache().then(async pool => {
            const page = parseInt(req.query.page as string) || 1;
            const search = req.query.query as string;
            const limit = parseInt(req.query.limit as string) || 6;
            if (search.length > 300) res.status(400).json({error: "no"})
            let tags: string[] = [];
            if (req.query.tags) {
                tags = req.query.tags.toString().split(",");
            }
            if (!tags.every(tag => allowedTags.includes(tag))) return res.status(400).json({error: `Tags must be one of: ${allowedTags.join(', ')}`});

            const offset = (page - 1) * limit;
            try {
                let query = `
                    SELECT
                        o.*,
                        u.name as account_name,
                        COALESCE(AVG(orate.stars), 0) as rating,
                        COUNT(orate.stars) as rating_count,
                        COUNT(*) OVER() AS total_records
                    FROM
                        objects o
                    LEFT JOIN
                        ratings orate ON o.id = orate.object_id
                    JOIN
                        users u ON o.account_id = u.account_id
                `;
                let orderBy = '';

                const queryParams: any[] = [search, limit, offset];

                if (tags.length > 0) {
                    query += `
                        WHERE o.tags @> $4 AND o.status = 1 AND (LOWER(o.name) LIKE LOWER('%' || $1 || '%') OR o.id::text = $1)
                    `;
                    queryParams.push(tags);
                } else {
                    query += `
                        WHERE o.status = 1 AND (LOWER(o.name) LIKE LOWER('%' || $1 || '%') OR o.id::text = $1)
                    `
                }

                query += `
                    GROUP BY o.id, u.name
                    ${orderBy}
                    LIMIT $2 OFFSET $3
                `;
                const result = await pool.query(query, queryParams);
                //const objectAmountRes = await pool.query(`SELECT count(*) AS count FROM objects ${(category == 3) ? orderBy : ""}`);
                //const objectAmount = parseInt(objectAmountRes.rows[0].count.toString());

                const totalRecords = (result.rows.length > 0) ? parseInt(result.rows[0].total_records) : 0;
                const totalPages = Math.ceil(totalRecords / limit);

                const objectData = convertRowsToObjects(result.rows)
                res.json({
                    results: objectData,
                    page,
                    total: totalRecords,
                    pageAmount: totalPages
                });
            } catch (e) {
                console.error(e);
                res.status(500).json({ error: 'Internal server error' });
            }
        }).catch(e => {
            console.error(e);
            res.status(500).json({ error: 'Internal server error' });
        });
    }
);

export default oRouter;
