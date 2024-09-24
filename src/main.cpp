#include <Geode/Geode.hpp>
using namespace geode::prelude;

#include "ui/ObjectWorkshop.hpp"
#include "ui/auth/AuthMenu.hpp"
#include "config.hpp"
#include "ui/auth/AuthLoadLayer.hpp"
#include <fig.authentication/include/authentication.hpp>
#include <alphalaneous.editortab_api/include/EditorTabs.hpp>

// 13 = custom objects
void CustomObjects::onWorkshop(CCObject*) {
    int authServer = Mod::get()->getSettingValue<int64_t>("auth-server");
    if (authServer != -1) {
        auto loadLayer = AuthLoadLayer::create();
        loadLayer->show();
        auto token = Mod::get()->getSettingValue<std::string>("token");
        AuthMenu::testAuth(token, [loadLayer, authServer, token](int value) {
            if (value == 1) {
                loadLayer->finished();
                ObjectWorkshop::create(true)->show();
            } else if (value == -1) {
                loadLayer->finished();
                FLAlertLayer::create("Error", "Currently, Object Workshop <cy>servers are down</c> at the moment! View your logs, or view announcements on the <cy>Discord Server</c> for more information, or if there are no announcements, inform the developer of this error!", "OK")->show();
            } else {
                switch (AuthMenu::intToAuth(authServer)) {
                    default:
                    case AuthMethod::None:
                    case AuthMethod::DashAuth: {
                        loadLayer->finished();
                        FLAlertLayer::create("Error", "Unsupported <cy>authentication method</c>.", "OK")->show();
                        break;
                    }
                    case AuthMethod::Custom: {
                        loadLayer->finished();
                        FLAlertLayer::create("Error", "Either the token you set is <cy>expired</c>, or you <cy>entered the token incorrectly!</c>", "OK")->show();
                        break;
                    }
                    case AuthMethod::GDAuth: {
                        authentication::AuthenticationManager::get()->getAuthenticationToken([loadLayer](std::string token) {
                            AuthMenu::genAuthToken(AuthMethod::GDAuth, token, false, [loadLayer](int value) {
                                loadLayer->finished();
                                if (value == 1) {
                                    ObjectWorkshop::create(true)->show();
                                } else if (value == -1) {
                                    FLAlertLayer::create("Error", "Currently, Object Workshop <cy>servers are down</c> at the moment! View your logs, or view announcements on the <cy>Discord Server</c> for more information, or if there are no announcements, inform the developer of this error!", "OK")->show();
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

bool CustomObjects::init(LevelEditorLayer* editorLayer) {
    if (!EditorUI::init(editorLayer)) return false;
    EditorTabs::addTab(this, TabType::BUILD, "workshop"_spr, [this](EditorUI* ui, CCMenuItemToggler* toggler) -> CCNode* { //create the tab
        auto folder = CCSprite::createWithSpriteFrameName("gj_folderBtn_001.png");
        folder->setScale(0.4F);
        auto labelC = CCLabelBMFont::create("C+", "bigFont.fnt");
        labelC->setScale(0.4F);
        folder->addChildAtPosition(labelC, Anchor::Center);
        CCLabelBMFont* textLabelOn = CCLabelBMFont::create("C+", "bigFont.fnt");
        textLabelOn->setScale(0.4f);
        CCLabelBMFont* textLabelOff = CCLabelBMFont::create("C+", "bigFont.fnt");
        textLabelOff->setScale(0.4f);

        EditorTabUtils::setTabIcons(toggler, folder, folder);

        m_fields->menu = CCMenu::create();
        m_fields->menu->setID("ow-menu"_spr);
        auto label = CCLabelBMFont::create("Custom Objects", "goldFont.fnt");
        label->setScale(0.525F);
        auto customObjsLabel = CCMenuItemSpriteExtra::create(label, this, nullptr);
        m_fields->menu->addChildAtPosition(customObjsLabel, Anchor::Top, {0, -9});

        m_fields->myObjsLabel = CCLabelBMFont::create("• My Objects", "bigFont.fnt");
        m_fields->myObjsLabel->setScale(0.4F);
        m_fields->myObjsLabel->setVisible(false);
        m_fields->menu->addChildAtPosition(m_fields->myObjsLabel, Anchor::Top, {57, -10});
        
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

        m_fields->btn1 = CCMenuItemSpriteExtra::create(btn1Spr, this, menu_selector(CustomObjects::onWorkshop));
        m_fields->menu->addChildAtPosition(m_fields->btn1, Anchor::Center, {0, -7});
        m_fields->menu->setContentHeight(90.F);
        m_fields->menu->updateLayout();

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
                    auto uploads = jsonRes.try_get<int>("uploads");
                    if (uploads) {
                        labelOther1->setString(
                            fmt::format(
                                "<cg>{}</c> Upload{} from you",
                                uploads.value(),
                                (uploads.value() == 1) ? "" : "s"
                            )
                        );
                    }
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
            req.userAgent(USER_AGENT);
            req.bodyJSON(myjson);
            m_fields->m_listener.setFilter(req.post(fmt::format("{}/user/@me", HOST_URL)));
        }
        auto winSize = CCDirector::sharedDirector()->getWinSize();
        m_fields->menu->setPosition({winSize.width / 2, 45});
        return m_fields->menu;
    });

    return true;
}

class $modify(ObjectBypass, EditorUI) {
    void onNewCustomItem(CCObject* pSender) {
        if (!Mod::get()->getSettingValue<bool>("object-bypass")) return EditorUI::onNewCustomItem(pSender);
        if (m_selectedObjects && m_selectedObjects->count() > 0) {
            if (auto gameManager = GameManager::sharedState()) {
                CCArray* newSelectedObjs;
                if (m_selectedObjects->count() == 0) {
                    newSelectedObjs = cocos2d::CCArray::create();
                    newSelectedObjs->addObject(m_selectedObject);
                } else {
                    newSelectedObjs = this->m_selectedObjects;
                }
                gameManager->addNewCustomObject(copyObjects(newSelectedObjs, false, false));
                m_selectedObjectIndex = 0;
                reloadCustomItems();
            }
        } else {
            EditorUI::onNewCustomItem(pSender);
        }

/*
        if (m_selectedObjects == nullptr || m_selectedObjects->count() <= 1000) {
            if (auto gameManager = GameManager::sharedState()) {
                if (gameManager->m_customObjectDict->count() < 200) {
                    if ((m_selectedObject == nullptr && m_selectedObjects->count() == 0) || (m_selectedObject != nullptr && m_selectedObject->m_objectID == 0x2ed)) return;
                    CCArray* newSelectedObjs;
                    if (m_selectedObjects->count() == 0) {
                        newSelectedObjs = cocos2d::CCArray::create();
                        newSelectedObjs->addObject(m_selectedObject);
                    } else {
                        newSelectedObjs = this->m_selectedObjects;
                    }
                    gameManager->addNewCustomObject(copyObjects(newSelectedObjs, false, false));
                    m_selectedObjectIndex = 0;
                    reloadCustomItems();
                } else {
                    auto string = cocos2d::CCString::createWithFormat("You cannot create more than <cy>%i</c> custom objects.", 200);
                    FLAlertLayer::create(nullptr, "Max Custom Objects", string->getCString(), "OK", nullptr)->show();
                }
            }
        } else {
            auto string = cocos2d::CCString::createWithFormat("A custom object cannot contain more than <cg>%i</c> objects.", 1000);
            FLAlertLayer::create(nullptr, "Max Limit", string->getCString(), "OK", nullptr)->show();
        }
*/
    }
};
