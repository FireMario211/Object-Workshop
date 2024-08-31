#include <Geode/Geode.hpp>
using namespace geode::prelude;

#include "ui/ObjectWorkshop.hpp"
#include "ui/AuthMenu.hpp"
#include "config.hpp"
#include <fig.authentication/include/authentication.hpp>

// 13 = custom objects
void CustomObjects::setupCustomMenu(EditButtonBar* bar, bool hideItems) {
    if (!Mod::get()->getSettingValue<bool>("enabled")) return;
    m_fields->oldChildrenCount = bar->getChildrenCount();
    if (auto boomlist = getChildOfType<BoomScrollLayer>(bar, 0)) {
        boomlist->setVisible(hideItems);
        boomlist->setPositionY(-30);
        boomlist->setScale(0.8F);
    }
    if (auto menu = getChildOfType<CCMenu>(bar, 0)) {
        menu->setVisible(hideItems);
    }
    auto menu = CCMenu::create();
    auto label = CCLabelBMFont::create("Custom Objects", "goldFont.fnt");
    label->setScale(0.525F);
    m_fields->customObjsLabel = CCMenuItemSpriteExtra::create(label, this, menu_selector(CustomObjects::onBackLbl));
    menu->addChildAtPosition(m_fields->customObjsLabel, Anchor::Top, {0, -9});

    m_fields->myObjsLabel = CCLabelBMFont::create("• My Objects", "bigFont.fnt");
    m_fields->myObjsLabel->setScale(0.4F);
    m_fields->myObjsLabel->setVisible(false);
    menu->addChildAtPosition(m_fields->myObjsLabel, Anchor::Top, {57, -10});
    
    //auto btn1 = ButtonSprite::create(CCSprite::create("GJ_button_04.png"), 64, true, 0, "GJ_button_01.png", 0.9F);
    auto btn1Spr = CCScale9Sprite::create("GJ_button_04.png");
    btn1Spr->setContentSize({90, 90});
    btn1Spr->setScale(0.7F);
    
    auto folder1 = CCSprite::createWithSpriteFrameName("gj_folderBtn_001.png");
    auto insideFolder = CCSprite::createWithSpriteFrameName("GJ_hammerIcon_001.png");
    insideFolder->setScale(0.6F);
    folder1->addChildAtPosition(insideFolder, Anchor::Center, {0, 0});

    auto label1 = CCLabelBMFont::create("Objects\nWorkshop", "bigFont.fnt");
    label1->setScale(0.375);
    label1->setAlignment(CCTextAlignment::kCCTextAlignmentCenter);

    auto bgOther1 = cocos2d::extension::CCScale9Sprite::create("square02_001.png", { 0.0f, 0.0f, 80.0f, 80.0f });
    bgOther1->setContentSize({66, 55});
    bgOther1->setScaleY(.35F);
    bgOther1->setOpacity(60);
    bgOther1->setColor({0,0,0});

    auto labelOther1 = TextArea::create(
        "<cg>0</c> Uploads from you",
        "bigFont.fnt",
        0.3F, 70.0F,
        ccp(0.4, 0),
        10.0F,
        false 
    );
    labelOther1->setAnchorPoint({0.53, 0.5});
    labelOther1->setScale(0.75F);
    btn1Spr->addChildAtPosition(folder1, Anchor::Center, {0, 23});
    btn1Spr->addChildAtPosition(label1, Anchor::Center, {0, -2});
    btn1Spr->addChildAtPosition(bgOther1, Anchor::Center, {0, -26});
    btn1Spr->addChildAtPosition(labelOther1, Anchor::Bottom, {0, 15});

    auto btn2Spr = CCScale9Sprite::create("GJ_button_04.png");
    btn2Spr->setContentSize({90, 90});
    btn2Spr->setScale(0.7F);

    auto folder2 = CCSprite::createWithSpriteFrameName("gj_folderBtn_001.png");
    folder2->setColor({255, 179, 107});

    auto label2 = CCLabelBMFont::create("My Objects", "bigFont.fnt");
    label2->setScale(0.375);

    auto bgOther2 = cocos2d::extension::CCScale9Sprite::create("square02_001.png", { 0.0f, 0.0f, 80.0f, 80.0f });
    bgOther2->setContentSize({70, 55});
    bgOther2->setScaleY(.5F);
    bgOther2->setOpacity(60);

    auto labelOther2 = TextArea::create(
        fmt::format("<cg>{}</c> Custom Objects", bar->m_buttonArray->count() - 4),
        "bigFont.fnt",
        0.3F, 70.0F,
        ccp(0.4, 0),
        10.0F,
        false 
    );
    labelOther2->setAnchorPoint({0.53, 0.5});

    btn2Spr->addChildAtPosition(folder2, Anchor::Center, {0, 23});
    btn2Spr->addChildAtPosition(label2, Anchor::Center, {0, 2});
    btn2Spr->addChildAtPosition(bgOther2, Anchor::Center, {0, -22});
    btn2Spr->addChildAtPosition(labelOther2, Anchor::Bottom, {0, 18});


    m_fields->btn1 = CCMenuItemSpriteExtra::create(btn1Spr, this, menu_selector(CustomObjects::onWorkshop));
    menu->addChildAtPosition(m_fields->btn1, Anchor::Center, {-36, -7});

    m_fields->btn2 = CCMenuItemSpriteExtra::create(btn2Spr, this, menu_selector(CustomObjects::onMyObjects));
    menu->addChildAtPosition(m_fields->btn2, Anchor::Center, {36, -7});

    menu->setContentHeight(90.F);
    menu->setPositionY(45.F);

    bar->addChild(menu);
    menu->updateLayout();

    if (hideItems) {
        m_fields->btn1->setVisible(false);
        m_fields->btn2->setVisible(false);
        m_fields->myObjsLabel->setVisible(true);
        m_fields->customObjsLabel->updateAnchoredPosition(Anchor::Top, {-44, -9});
    }

    auto token = Mod::get()->getSettingValue<std::string>("token");
    if (token != "" && !m_fields->hasCheckedUploads) {
        m_fields->hasCheckedUploads = true;
        m_fields->m_listener.bind([this, labelOther1] (web::WebTask::Event* e) {
            if (web::WebResponse* value = e->getValue()) {
                if (value->json().has_error()) {
                    log::error("Invalid server response.");
                    Notification::create("Object Workshop server gave invalid response.", NotificationIcon::Warning)->show();
                    return;
                }
                auto jsonRes = value->json().unwrapOrDefault();
                if (!jsonRes.is_object()) return log::error("Response isn't object.");
                auto jsonObj = jsonRes.as_object();
                auto isError = jsonRes.try_get<std::string>("error");
                if (isError) {
                    log::error("{}", isError);
                    return;
                }
                int uploads = jsonRes.get<int>("uploads");
                labelOther1->setString(
                    fmt::format(
                        "<cg>{}</c> Upload{} from you",
                        uploads,
                        (uploads == 1) ? "" : "s"
                    )
                );
            } else if (web::WebProgress* progress = e->getProgress()) {
                // The request is still in progress...
            } else if (e->isCancelled()) {
                log::error("Request was cancelled.");
            }
        });
        web::WebRequest req = web::WebRequest();
        auto myjson = matjson::Value();
        myjson.set("token", token);
        req.header("Content-Type", "application/json");
        req.bodyJSON(myjson);
        m_fields->m_listener.setFilter(req.post(fmt::format("{}/user/@me", HOST_URL)));
    }
}

void CustomObjects::setupCreateMenu() {
    EditorUI::setupCreateMenu();
    if (auto bar = getChildOfType<EditButtonBar>(this, 13)) {
    //if (auto bar = typeinfo_cast<EditButtonBar*>(getChildByID("custom-tab-bar"))) {
        CustomObjects::setupCustomMenu(bar, false);
    }
}
void CustomObjects::reloadCustomItems() { // rebuild it!
    EditorUI::reloadCustomItems();
    if (auto bar = typeinfo_cast<EditButtonBar*>(getChildByID("custom-tab-bar"))) {
        CustomObjects::setupCustomMenu(bar, true);
    }
}
void CustomObjects::onBackLbl(CCObject*) {
    if (auto bar = typeinfo_cast<EditButtonBar*>(getChildByID("custom-tab-bar"))) {
        if (auto boomlist = getChildOfType<BoomScrollLayer>(bar, 0)) {
            boomlist->setVisible(false);
        }
        if (m_fields->oldChildrenCount >= 2) {
            if (auto menu = getChildOfType<CCMenu>(bar, 0)) {
                menu->setVisible(false);
            }
        }
        m_fields->btn1->setVisible(true);
        m_fields->btn2->setVisible(true);
        m_fields->myObjsLabel->setVisible(false);
        m_fields->customObjsLabel->updateAnchoredPosition(Anchor::Top, {0, -9});
    }
}
void CustomObjects::onWorkshop(CCObject*) {
    int authServer = Mod::get()->getSettingValue<int64_t>("auth-server");
    if (authServer != -1) {
        auto token = Mod::get()->getSettingValue<std::string>("token");
        AuthMenu::testAuth(token, [authServer, token](bool value) {
            if (value) {
                ObjectWorkshop::create(true)->show();
            } else {
                switch (AuthMenu::intToAuth(authServer)) {
                    default:
                    case AuthMethod::None:
                    case AuthMethod::DashAuth: {
                        FLAlertLayer::create("Error", "Unsupported <cy>authentication method</c>.", "OK")->show();
                        break;
                    }
                    case AuthMethod::GDAuth: {
                        authentication::AuthenticationManager::get()->getAuthenticationToken([](std::string token) {
                            AuthMenu::genAuthToken(AuthMethod::GDAuth, token, false, [](bool value) {
                                if (value) {
                                    ObjectWorkshop::create(true)->show();
                                } else {
                                    FLAlertLayer::create("Error", "Something went wrong when <cy>trying to generate a new authentication token!</c>\nIf this issue happens again, please consider <cr>resetting your settings</c> to redo the authentication process.", "OK")->show();
                                }
                            });
                        });
                        break;
                    }
                }
            }
        });
    } else {
        AuthMenu::create()->show();
    }
}
void CustomObjects::onMyObjects(CCObject*) {
    if (auto bar = typeinfo_cast<EditButtonBar*>(getChildByID("custom-tab-bar"))) {
        if (auto boomlist = getChildOfType<BoomScrollLayer>(bar, 0)) {
            boomlist->setVisible(true);
        }
        if (auto menu = getChildOfType<CCMenu>(bar, 0)) {
            menu->setVisible(true);
        }
        m_fields->btn1->setVisible(false);
        m_fields->btn2->setVisible(false);
        m_fields->myObjsLabel->setVisible(true);
        m_fields->customObjsLabel->updateAnchoredPosition(Anchor::Top, {-44, -9});
    }
}

/*
cocos2d::CCSprite* spriteFromObjectString(gd::string p0, bool p1, bool p2, int p3, cocos2d::CCArray *p4, cocos2d::CCArray *p5, GameObject *p6) {
    //log::info("{} - {},{},{},{},{}",p0,p1,p2,p3,p4,p5,p6);
    return EditorUI::spriteFromObjectString(p0,p1,p2,p3,p4,p5,p6);
}*/

/*
#include <Geode/modify/LevelEditorLayer.hpp>

class $modify(LevelEditorLayer) {
    CCArray* createObjectsFromString(gd::string& str, bool p1, bool p2) {
        //geode::log::info("create {} {} {}", p1, p2, str);
        return LevelEditorLayer::createObjectsFromString(str, p1, p2);
	}
};
*/
