#pragma once
#include <Geode/Geode.hpp>
#include <Geode/ui/Popup.hpp>
#include "../../config.hpp"
#include "../../utils.hpp"
#include "Geode/utils/web.hpp"
using namespace geode::prelude;

enum CaseType {
    Warning = 0,
    TCommentBan = 1,
    TUploadBan = 2,
    TBan = 3,
    CommentBan = 4,
    UploadBan = 5,
    Ban = 6,
};

struct CaseData {
    int id;
    int number = 0;
    CaseType type;
    std::string reason;
    std::string timestamp;
    int account_id;
    int staff_account_id;
    std::string expiration;
    bool ack;
    std::string ack_timestamp;
    std::string staff_account_name;
};


template<>
struct matjson::Serialize<CaseData> {
    static Result<CaseData> fromJson(matjson::Value const& value) {
        CaseData data;
        // note for me, GEODE_UNWRAP_INTO will automatically return Err if something happened, aka IN THIS FUNC
        GEODE_UNWRAP_INTO(data.id, value["case_id"].asInt());
        GEODE_UNWRAP_INTO(auto case_type, value["case_type"].asInt());
        GEODE_UNWRAP_INTO(data.account_id, value["account_id"].asInt());
        GEODE_UNWRAP_INTO(data.staff_account_id, value["staff_account_id"].asInt());
        GEODE_UNWRAP_INTO(data.reason, value["case_type"].asString());
        GEODE_UNWRAP_INTO(data.timestamp, value["case_type"].asString());
        GEODE_UNWRAP_INTO(data.expiration, value["case_type"].asString());
        GEODE_UNWRAP_INTO(data.ack, value["case_type"].asBool());
        GEODE_UNWRAP_INTO(data.ack_timestamp, value["case_type"].asString());
        GEODE_UNWRAP_INTO(data.staff_account_name, value["staff_account_name"].asString());
        data.type = static_cast<CaseType>(case_type);
        return Ok(data);
    }
    static matjson::Value toJson(CaseData const& value) {
        // is this even necessary
        matjson::Value obj;
        obj.set("case_id", value.id);
        obj.set("case_type", static_cast<int>(value.type));
        obj.set("account_id", value.account_id);
        obj.set("staff_account_id", value.staff_account_id);
        obj.set("reason", value.reason);
        obj.set("timestamp", value.timestamp);
        obj.set("expiration", value.expiration);
        obj.set("ack", value.ack);
        obj.set("ack_timestamp", value.ack_timestamp);
        obj.set("staff_account_name", value.staff_account_name);
        return obj;
    }
};

class WarningPopup : public geode::Popup<CaseData, std::function<void()>> {
protected:
    EventListener<web::WebTask> m_listener;
    CaseData m_case;
    bool setup(CaseData, std::function<void()>) override;
    void keyDown(cocos2d::enumKeyCodes) override {};
    virtual void keyBackClicked() override {};
    void onAcknowledge(CCObject*);
public:
    static WarningPopup* create(CaseData caseData, std::function<void()> callback) {
        auto ret = new WarningPopup();
        if (ret->initAnchored(260.f, 220.f, caseData, callback)) {
            ret->autorelease();
            return ret;
        }
        delete ret;
        return nullptr;
    }
};
