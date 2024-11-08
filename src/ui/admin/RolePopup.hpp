#pragma once

#include <Geode/ui/Popup.hpp>
#include "../ObjectWorkshop.hpp"
#include "../../config.hpp"

using namespace geode::prelude;

class RolePopup : public geode::Popup<UserData, std::function<void(int)>> {
protected:
    std::vector<CCMenuItemSpriteExtra*> m_roleBtns;
    int m_selectedRole;
    UserData m_user;
    bool setup(UserData, std::function<void(int)>) override;
    std::function<void(int)> m_submitCallback;

    virtual void onClose(CCObject* sender) override;
public:
    static RolePopup* create(UserData user, std::function<void(int)> callback) {
        auto ret = new RolePopup();
        if (ret->initAnchored(50.F * ROLE_COUNT, 120.f, user, callback)) {
            ret->autorelease();
            return ret;
        }
        delete ret;
        return nullptr;
    }
};
