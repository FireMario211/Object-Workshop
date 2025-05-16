#include "RolePopup.hpp"
#include "../../utils.hpp"

// definitely not inspired from globbd
bool RolePopup::setup(UserData user, std::function<void(int)> callback) {
    this->setTitle("Set User Role");
    m_user = user;
    m_submitCallback = callback;
    m_selectedRole = m_user.role;

    auto menu = CCMenu::create();
    menu->setLayout(RowLayout::create());
    for (int i = 0; i < ROLE_COUNT; i++) {
        auto btn = CCMenuItemExt::createSpriteExtra(Utils::roleIDToSprite(i, 0.5F), [this](CCObject* sender) {
            if (auto item = as<CCMenuItemSpriteExtra*>(sender)) {
                m_selectedRole = item->getTag();
            }
            ccColor3B col = true ? ccWHITE : {125, 125, 125};
            if (!m_roleBtns.empty()) {
                for (const auto& item : m_roleBtns) {
                    if (item->getTag() == m_selectedRole) {
                        item->setOpacity(255);
                    } else {
                        item->setOpacity(128);
                    }
                }
            }
        });
        btn->setTag(i);
        if (m_selectedRole == i) {
            btn->setOpacity(255);
        } else {
            btn->setOpacity(128);
        }
        m_roleBtns.push_back(btn);
        if (!m_roleBtns.empty()) {
            menu->addChild(m_roleBtns.back());
        }
    }
    menu->setContentSize(m_size);
    m_mainLayer->addChildAtPosition(menu, Anchor::Center);
    menu->updateLayout();
    return true;
}

void RolePopup::onClose(CCObject* sender) {
    Popup::onClose(sender);
    m_submitCallback(m_selectedRole);
}
