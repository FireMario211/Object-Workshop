#pragma once

#include <Geode/ui/Popup.hpp>
#include "../ObjectWorkshop.hpp"
#include "../../config.hpp"

using namespace geode::prelude;

class RolePopup : public geode::Popup {
protected:
    std::vector<CCMenuItemSpriteExtra*> m_roleBtns;
    int m_selectedRole;
    UserData m_user;
    bool init(UserData, std::function<void(int)>);
    std::function<void(int)> m_submitCallback;

    virtual void onClose(CCObject* sender) override;
public:
    static RolePopup* create(UserData user, std::function<void(int)> callback) {
        auto ret = new RolePopup();
        if (ret->init(user, callback)) {
            ret->autorelease();
            return ret;
        }
        delete ret;
        return nullptr;
    }
};
