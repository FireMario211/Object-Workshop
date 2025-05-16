import { Pool, PoolClient } from 'pg';
let pool: PoolClient;
let isReady: boolean;

async function getCache(): Promise<PoolClient> {
    if (!isReady) {
        let apool = new Pool({
            max: 20,
            idleTimeoutMillis: 20000,
            connectionTimeoutMillis: 2000,
            user: 'myuser',
            host: 'localhost',
            database: 'db',
            password: 'mypassword',
            port: 5432
        });
        apool.on('error', (err: Error) => {
            console.error(`[Postgres] Error: ${err}`)
            isReady = false;
        })
        apool.on('connect', () => console.log('[Postgres] Connected!'))
        apool.on('acquire', () => console.log('[Postgres] Connection acquired'))
        apool.on('remove', () => console.log('[Postgres] Connection removed'))
        try {
            pool = await apool.connect();
            isReady = true;
            console.log("[Postgres] Ready!");
            return pool;
        } catch (err) {
            console.error("Failed to connect to Postgres", err);
            throw err;
        }
    } else {
        return pool;
    }
}
export { getCache }
