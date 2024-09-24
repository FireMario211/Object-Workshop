export default interface ObjectData {
    id: number;
    account_id: number;
    account_name: string;
    name: string;
    description: string;
    downloads: number;
    favorites: number;
    rating: number;
    rating_count: number;
    tags: Array<string>;
    featured: number;
    status: number;
    version: number;
    created: string;
    updated: string;
    data: string;
};
