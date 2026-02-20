#pragma once

#include <Geode/ui/Popup.hpp>
#include "../ObjectWorkshop.hpp"
#include "../../config.hpp"

using namespace geode::prelude;

class CasePopup : public geode::Popup {
protected:
    async::TaskHolder<geode::utils::web::WebResponse> m_listener;
    UserData m_user;
    UserData m_managingUser;
    CCScale9Sprite* m_casesBG;
    bool init(UserData, UserData);

    void onLoadCases();

    virtual void onClose(CCObject* sender) override;
public:
    static CasePopup* create(UserData user1, UserData user2) {
        auto ret = new CasePopup();
        if (ret->init(user1, user2)) {
            ret->autorelease();
            return ret;
        }
        delete ret;
        return nullptr;
    }
};
