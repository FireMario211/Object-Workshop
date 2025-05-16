import { Request, Response, NextFunction } from 'express'
import { TimedMap } from './Components/TimedMap';

type ShouldCacheRoute = (req: Request) => boolean;

export class CacheManager {
    private static cache = new TimedMap<string, any>(60000, 1000);
    static get<T>(key: string): T | null {
        return this.cache.get(key);
    }
    static set<T>(key: string, data: T, ttl: number): void {
        this.cache.set(key, data, ttl * 1000);
    }
    static delete(key: string): boolean {
        return this.cache.delete(key);
    }
    static deletePattern(key: string): void {
        this.cache.deleteByPattern(key);
    }
}

export const cacheMiddleware = (ttl: number, cacheRoute: ShouldCacheRoute = () => true) => {
    return (req: Request, res: Response, next: NextFunction) => {
        if (!cacheRoute(req)) return next();
        const key = `${req.method}:${req.originalUrl}`;
        const data = CacheManager.get(key);
        if (data) return res.json(data);
        const origData = res.json;
        res.json = function(body: any) {
            if (res.statusCode >= 200 && res.statusCode < 300) {
                CacheManager.set(key, body, ttl);
            }
            return origData.call(this, body);
        };
        next();
    }
}
