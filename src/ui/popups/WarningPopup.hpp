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
    int number;
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
