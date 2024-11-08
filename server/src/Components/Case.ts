export default interface CaseData {
    id: number;
    case_type: number;
    account_id: number;
    staff_account_id: number;
    reason: string;
    timestamp: Date | number;
    expiration: Date | number;
    ack: boolean;
    ack_timestamp: Date | number;
};
