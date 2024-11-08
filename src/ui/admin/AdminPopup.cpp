#include "AdminPopup.hpp"
#include "RolePopup.hpp"
#include "CasePopup.hpp"
#include "../../utils.hpp"
#include "../../config.hpp"

bool AdminPopup::setup(UserData user, UserData managingUser) {
    m_user = user;
    m_managingUser = managingUser;
    auto nameLabel = CCLabelBMFont::create(managingUser.name.c_str(), "bigFont.fnt");
    nameLabel->limitLabelWidth(160.F, 0.8F, 0.1F);
    m_mainLayer->addChildAtPosition(nameLabel, Anchor::Top, {0, -20});
    m_buttonMenu->addChildAtPosition(
        CCMenuItemExt::createSpriteExtra(Utils::roleIDToSprite(m_managingUser.role, 0.6F), [this](CCObject* sender) {
            if (m_managingUser.role < 0) return FLAlertLayer::create("Error", "You cannot <cy>set a role</c> for this user as they are currently <cr>banned</c>!", "OK")->show();
            //if (m_managingUser.role >= m_user.role) return FLAlertLayer::create("Error", "You cannot <cy>set a role</c> for this user as they are a <cg>higher role than you</c>!", "OK")->show();
            RolePopup::create(m_managingUser, [this, sender](int role) {
                if (auto item = typeinfo_cast<CCMenuItemSpriteExtra*>(sender)) {
                    m_managingUser.role = role;
                    m_hadSetRole = true;
                    item->setSprite(Utils::roleIDToSprite(m_managingUser.role, 0.6F));
                }
            })->show();
        }),
        Anchor::TopRight,
        {-20, -20}
    );
    m_buttonMenu->addChildAtPosition(
        CCMenuItemExt::createSpriteExtra(ButtonSprite::create("View Cases", "bigFont.fnt", "GJ_button_04.png", 0.6F), [this](CCObject*) {
            CasePopup::create(m_user, m_managingUser)->show();
        }),
        Anchor::Center,
        { 0, 10 }
    );
    m_buttonMenu->addChildAtPosition(
        CCMenuItemExt::createSpriteExtra(ButtonSprite::create("Update", "bigFont.fnt", "GJ_button_01.png", 1.0F), [this](CCObject*) {
            if (!m_hadSetRole) return FLAlertLayer::create("Error", "There is nothing to update!", "OK")->show();
        }),
        Anchor::Bottom,
        { 0, 30 }
    );
    return true;
}
