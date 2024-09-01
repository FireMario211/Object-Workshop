export default interface ObjectData {
    id: number;
    account_id: number;
    account_name: string;
    timestamp: Date | number;
    name: string;
    description: string;
    downloads: number;
    favorites: number;
    rating: number;
    rating_count: number;
    tags: Array<string>;
    status: number;
    data: string;
};
