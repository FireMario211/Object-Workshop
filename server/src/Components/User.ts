export interface UserData {
    auth_method: string;
    account_id: number;
    name: string;
    downloaded: Array<number>;
    favorites: Array<number>;
    timestamp: Date | number;
    role: number;
    uploads: number;
};
