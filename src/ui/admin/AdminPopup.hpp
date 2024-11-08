#pragma once

#include <Geode/ui/Popup.hpp>
#include "../ObjectWorkshop.hpp"
#include <Geode/utils/web.hpp>
using namespace geode::prelude;

class AdminPopup : public geode::Popup<UserData, UserData> {
protected:
    EventListener<web::WebTask> m_listener;
    UserData m_managingUser;
    UserData m_user;
    bool setup(UserData, UserData) override;
    bool m_hadSetRole = false;
public:
    static AdminPopup* create(UserData user1, UserData user2) {
        auto ret = new AdminPopup();
        if (ret->initAnchored(300.f, 150.f, user1, user2)) {
            ret->autorelease();
            return ret;
        }

        delete ret;
        return nullptr;
    }
};
