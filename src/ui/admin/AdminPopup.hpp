#pragma once

#include <Geode/ui/Popup.hpp>
#include "../ObjectWorkshop.hpp"
#include <Geode/utils/web.hpp>
using namespace geode::prelude;

class AdminPopup : public geode::Popup {
protected:
    async::TaskHolder<geode::utils::web::WebResponse> m_listener;
    UserData m_managingUser;
    UserData m_user;
    bool init(UserData, UserData);
    bool m_hadSetRole = false;
public:
    static AdminPopup* create(UserData user1, UserData user2) {
        auto ret = new AdminPopup();
        if (ret->init(user1, user2)) {
            ret->autorelease();
            return ret;
        }

        delete ret;
        return nullptr;
    }
};
