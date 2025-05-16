// copied from https://github.com/FireMario211/dashend-min/blob/main/src/utils.ts except sorta improved !!!
// the amount of time it took for me to come up with this is insane
export interface TimedEntry<T> {
    value: T;
    expiration: number;
    created: number;
}

export class TimedMap<K, V> {
    private map: Map<K, TimedEntry<V>> = new Map();
    private max: number;
    private cleanInterval: NodeJS.Timeout | null = null;
    constructor(interval: number, max = Infinity) {
        this.max = max;
        if (interval && interval > 0) {
            this.cleanInterval = setInterval(() => this.cleanup(), interval);
        }
    }
    set(key: K, value: V, ttl: number): void {
        if (this.map.size >= this.max) {
            this.removeOldest();
        }
        const created = Date.now();
        const expiration = created + ttl;
        this.map.set(key, { value, expiration, created });
    }
    get(key: K): V | null {
        const entry = this.map.get(key);
        if (entry) {
            if (Date.now() < entry.expiration) {
                return entry.value;
            } else {
                this.map.delete(key);
            }
        }
        return null;
    }
    delete(key: K): boolean {
        return this.map.delete(key);
    }
    deleteByPattern(pattern: string): void {
        for (const key of this.map.keys()) {
            const keyStr = String(key);
            if (keyStr.startsWith(pattern)) {
                this.map.delete(key);
            }
        }
    }
    find(fn: (value: TimedEntry<V>, key: K) => boolean): [K, TimedEntry<V>] | null {
        for (const [key, value] of this.map.entries()) {
            if (fn(value, key)) {
                return [key, value];
            }
        }
        return null;
    }
    cleanup(): void {
        const now = Date.now();
        for (const [key, value] of this.map.entries()) {
            if (now >= value.expiration) {
                this.delete(key);
            }
        }
    }
    removeOldest(): boolean {
        if (this.map.size == 0) return false;
        let oldestKey: K | null = null;
        let oldestCreated: number = Infinity;
        for (const [key, value] of this.map.entries()) {
            if (value.created < oldestCreated) {
                oldestCreated = value.created;
                oldestKey = key;
            }
        }
        if (oldestKey != null) {
            return this.delete(oldestKey);
        }
        return false;
    }
}
