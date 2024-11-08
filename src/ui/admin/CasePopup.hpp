#pragma once

#include <Geode/ui/Popup.hpp>
#include "../ObjectWorkshop.hpp"
#include "../../config.hpp"

using namespace geode::prelude;

class CasePopup : public geode::Popup<UserData, UserData> {
protected:
    EventListener<web::WebTask> m_listener;
    UserData m_user;
    UserData m_managingUser;
    CCScale9Sprite* m_casesBG;
    bool setup(UserData, UserData) override;

    void onLoadCases(CCObject*);

    virtual void onClose(CCObject* sender) override;
public:
    static CasePopup* create(UserData user1, UserData user2) {
        auto ret = new CasePopup();
        if (ret->initAnchored(250.f, 200.f, user1, user2)) {
            ret->autorelease();
            return ret;
        }
        delete ret;
        return nullptr;
    }
};
