// please do not cry of this code

import ObjectData from '@/Components/Object';
import { getCache } from '../postgres';
import { Router, Request, Response } from 'express'
import { query, body, param, validationResult, CustomValidator } from 'express-validator';
import { verifyToken } from './user';

const allowedTags = ["Font", "Decoration", "Gameplay", "Art", "Structure", "Custom"];

const oRouter = Router();

/*
async function rateObject(redisClient: RedisClientType, objectID: string, userID: string, rating: number) {
    const ratingKey = `objects:${objectID}:ratings`;
    await redisClient.hSet(ratingKey, userID, rating);
}

async function calculateAverageRating(redisClient: RedisClientType, objectID: number): Promise<number> {
    const ratingKey = `objects:${objectID}:ratings`;
    const ratings = await redisClient.hGetAll(ratingKey);
    const totalRatings: number = Object.values(ratings).reduce((acc, rating) => acc + parseInt(rating), 0);
    const numberOfRatings = Object.keys(ratings).length;
    
    return numberOfRatings > 0 ? totalRatings / numberOfRatings : 0;
}
async function calculateAverageRating(pool: PoolClient, articleID: number): Promise<number> {
    const query = `
        SELECT AVG(rating) AS average_rating
        FROM ratings
        WHERE article_id = $1;
    `;
    const values = [articleID];

    try {
        const res = await pool.query(query, values);
        const averageRating = parseFloat(res.rows[0].average_rating);
        return isNaN(averageRating) ? 0 : averageRating;
    } catch (err: any) {
        console.error('Error executing query', err.stack);
        throw err;
    }
}

function dataToObjectData(redisClient: RedisClientType, data: ObjectData): Promise<ObjectData> {
    return new Promise((resolve, reject) => {
        data.timestamp = new Date(data.timestamp as number);
        data.tags = data.tags.toString().split(",");
        calculateAverageRating(redisClient, data.id).then(avgRatings => {
            data.rating = avgRatings;
            resolve(data);
        }).catch(reject);
    })
}*/ 

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
        timestamp: new Date(row.timestamp),
        name: row.name,
        description: row.description,
        downloads: row.downloads,
        favorites: row.favorites,
        rating_count: parseInt(row.rating_count.toString()),
        rating: parseFloat(row.rating.toString()),
        tags: row.tags,
        status: row.status,
        data: row.data
    };
}

function convertRowsToObjects(rows: any): Array<ObjectData> {
    return rows.map(convertRowToObject);
}

const blacklistedObjectIDs = [142];

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
        if (description.length > 300) return res.status(413).json({error: "The description cannot be more than 300 characters long!"});
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
        if (!tags || !tags.length) tags = [];
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
                try {
                    const dupCheck = await pool.query("SELECT id FROM objects WHERE data = $1 LIMIT 1;", [data]);
                    if (dupCheck.rowCount != null && dupCheck.rowCount > 0) return res.status(409).json({error: "You cannot upload an object that already exists!"});
                    const insertQuery = `
                        INSERT INTO objects (account_id, name, description, tags, data)
                        VALUES ($1, $2, $3, $4, $5)
                        RETURNING id, timestamp;
                    `;
                    const insertResult = await pool.query(insertQuery, [verifyRes.user.account_id, name, description, tags, data]);
                    if (insertResult.rowCount === 1) {
                        const object = insertResult.rows[0];
                        res.status(200).json({
                            id: object.id,
                            name,
                            description,
                            tags,
                            timestamp: object.timestamp
                        });
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
/*
oRouter.post('/:room/upload', validator.body('username').notEmpty().isString(), validator.body('ext').notEmpty().isString(), validator.body('file').notEmpty().isString(), (req, res) => {
        const roomID = req.params.room;
        if (!roomID) res.sendStatus(400);
        const result = validator.validationResult(req);
        if (!result.isEmpty()) return res.status(400).json({ errors: result.array() })
        uploadImage(req, res, false)
    })
})
*/ 

oRouter.get('/objects/:id', param("id").isNumeric().notEmpty(), (req: Request, res: Response) => {
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
    param('id').isNumeric().notEmpty(),
    body('token').notEmpty().isString(),
    body('stars').notEmpty().isNumeric(),
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
    param('id').isNumeric().notEmpty(),
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
                    let query2 = "";
                    let message = ""
                    if (verifyRes.user?.favorites.includes(parseInt(objectID))) {
                        query = "UPDATE users SET favorites = ARRAY_REMOVE(favorites, $1) WHERE account_id = $2";
                        query2 = "UPDATE objects SET favorites = favorites - 1 WHERE id = $1";
                        message = "Unfavorited object!";
                    } else {
                        query = "UPDATE users SET favorites = ARRAY_APPEND(favorites, $1) WHERE account_id = $2";
                        query2 = "UPDATE objects SET favorites = favorites + 1 WHERE id = $1";
                        message = "Favorited object!";
                    }
                    await pool.query(query, [objectID, accountID]);
                    await pool.query(query2, [objectID]);
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
    param('id').isNumeric().notEmpty(),
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
                    if (verifyRes.user?.favorites.includes(parseInt(objectID))) {
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
    param('id').isNumeric().notEmpty(),
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
                        const objExists = await pool.query("SELECT EXISTS (SELECT 1 FROM objects WHERE id = $1)", [objectID])
                        if (!objExists.rows[0].exists) return res.status(404).json({error: "Object not found."}); 
                    } else {
                        const objExists = await pool.query("SELECT EXISTS (SELECT 1 FROM objects WHERE id = $1 AND account_id = $2)", [objectID, accountID])
                        if (!objExists.rows[0].exists) return res.status(404).json({error: "Object not found."}); 
                    }
                    await pool.query("DELETE FROM objects WHERE id = $1", [objectID]);
                    res.status(200).json({ message: "Deleted object!" });
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
    param('id').isNumeric().notEmpty(),
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
        if (description.length > 300) return res.status(413).json({error: "The description cannot be more than 300 characters long!"});
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
                    if (verifyRes.user && verifyRes.user.role == 3) {
                        const objExists = await pool.query("SELECT EXISTS (SELECT 1 FROM objects WHERE id = $1)", [objectID])
                        if (!objExists.rows[0].exists) return res.status(404).json({error: "Object not found."}); 
                    } else {
                        const objExists = await pool.query("SELECT EXISTS (SELECT 1 FROM objects WHERE id = $1 AND account_id = $2)", [objectID, accountID])
                        if (!objExists.rows[0].exists) return res.status(404).json({error: "Object not found."}); 
                    }
                    const query = `
                        UPDATE objects SET
                        name = $1,
                        description = $2,
                        tags = $3 WHERE id = $4
                    `;
                    await pool.query(query, [name, description, tags, objectID]);
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
    param('id').isNumeric().notEmpty(),
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
                if (verifyRes.user && verifyRes.user.role < 2) return res.status(403).json({ error: "No permission" });
                try {
                    const objExists = await pool.query("SELECT EXISTS (SELECT 1 FROM objects WHERE id = $1 AND status = 0)", [objectID])
                    if (!objExists.rows[0].exists) return res.status(404).json({error: "Object not found."});
                    await pool.query("UPDATE objects SET status = 1 WHERE id = $1", [objectID]);
                    res.status(200).json({ message: "Accepted!" });
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
    param('id').isNumeric().notEmpty(),
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
                if (verifyRes.user && verifyRes.user.role < 2) return res.status(403).json({ error: "No permission" });
                try {
                    const objExists = await pool.query("SELECT EXISTS (SELECT 1 FROM objects WHERE id = $1 AND status = 0)", [objectID])
                    if (!objExists.rows[0].exists) return res.status(404).json({error: "Object not found."});
                    await pool.query("DELETE FROM objects WHERE id = $1", [objectID]);
                    res.status(200).json({ message: "Rejected!" });
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


oRouter.post('/user/@me/objects',
    body('token').notEmpty().isString(),
    query('page').isNumeric().optional(),
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
                const limit = (req.query.limit as string == "true") ? 2 : 9;
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

oRouter.post('/objects/pending',
    body('token').notEmpty().isString(),
    query('page').isNumeric().optional(),
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
                if (verifyRes.user && verifyRes.user.role < 2) return res.status(403).json({ error: "No permission" });
                const page = parseInt(req.query.page as string) || 1;
                const limit = (req.query.limit as string == "true") ? 2 : 9;
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
                        ORDER BY o.timestamp DESC
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

oRouter.post('/user/@me/favorites',
    body('token').notEmpty().isString(),
    query('page').isNumeric().optional(),
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
    query('page').isNumeric().optional(),
    query('category').isNumeric().optional(),
    body('tags').optional().isString(),
    async (req: Request, res: Response) => {
        const result = validationResult(req);
        if (!result.isEmpty()) return res.status(400).json({ errors: result.array() })
        getCache().then(async pool => {
            const page = parseInt(req.query.page as string) || 1;
            const category = parseInt(req.query.category as string) || 0;
            let tags: string[] = [];
            if (req.query.tags) {
                tags = req.query.tags.toString().split(",");
            }
            if (!tags.every(tag => allowedTags.includes(tag))) return res.status(400).json({error: `Tags must be one of: ${allowedTags.join(', ')}`})

            const limit = 6;
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
                        orderBy = 'ORDER BY rating DESC, rating_count DESC';
                        break;
                    case 2: // Most Liked
                        orderBy = 'ORDER BY o.favorites DESC';
                        break;
                    /*case 3: // Trending (based on the current week)
                        //WHERE o.timestamp >= NOW() - INTERVAL '7 days'
                        query += `
                            AND o.timestamp >= COALESCE((SELECT MAX(timestamp) FROM objects), NOW() - INTERVAL '7 days')
                        `;
                        orderBy = 'ORDER BY o.downloads DESC';
                        break;*/
                    case 3: // Most recent
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
    query('page').isNumeric().optional(),
    body('tags').optional().isString(),
    async (req: Request, res: Response) => {
        const result = validationResult(req);
        if (!result.isEmpty()) return res.status(400).json({ errors: result.array() })
        getCache().then(async pool => {
            const page = parseInt(req.query.page as string) || 1;
            const search = req.query.query as string;
            if (search.length > 300) res.status(400).json({error: "no"})
            let tags: string[] = [];
            if (req.query.tags) {
                tags = req.query.tags.toString().split(",");
            }
            if (!tags.every(tag => allowedTags.includes(tag))) return res.status(400).json({error: `Tags must be one of: ${allowedTags.join(', ')}`});

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
                `;
                let orderBy = '';

                const queryParams: any[] = [search, limit, offset];

                if (tags.length > 0) {
                    query += `
                        WHERE o.tags @> $4 AND o.status = 1 AND LOWER(o.name) LIKE LOWER($1 || '%')
                    `;
                    queryParams.push(tags);
                } else {
                    query += `
                        WHERE o.status = 1 AND LOWER(o.name) LIKE LOWER($1 || '%')
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