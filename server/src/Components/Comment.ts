export default interface CommentData {
    id: number;
    objectID: number;
    accountID: number;
    timestamp: Date | number;
    content: string;
    likes: number;
};
