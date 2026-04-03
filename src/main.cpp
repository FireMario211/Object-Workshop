#include <Geode/Geode.hpp>
using namespace geode::prelude;

#include "ui/ObjectWorkshop.hpp"
#include "ui/auth/AuthMenu.hpp"
#include "config.hpp"
#include "ui/auth/AuthLoadLayer.hpp"
#include <alphalaneous.editortab_api/include/EditorTabAPI.hpp>

// 13 = custom objects
void CustomObjects::onWorkshop(CCObject*) {
    int authServerA = Mod::get()->getSettingValue<int64_t>("auth-server");
    if (authServerA != -1) {
        auto loadLayer = AuthLoadLayer::create();
        loadLayer->show();
        auto token = Mod::get()->getSettingValue<std::string>("token");
        AuthMenu::testAuth(token, [this, loadLayer, authServerA, token](int value) {
            int authServer = authServerA;
            if (value == 1) {
                loadLayer->finished();
                ObjectWorkshop::create(true)->show();
            } else if (value == -1) {
                loadLayer->finished();
                FLAlertLayer::create("Error", "Currently, Object Workshop <cy>servers are down</c> at the moment! View your logs, or view announcements on the <cy>Discord Server</c> for more information, or if there are no announcements, inform the developer of this error!", "OK")->show();
            } else {
#if !defined(ARGON) && defined(DASHAUTH)
    authServer = 0; // fallback
    log::info("Argon disabled, fallback to DashAuth");
#endif
#if defined(ARGON) && !defined(DASHAUTH)
    authServer = 1;
    log::info("DashAuth disabled, fallback to Argon");
#endif
                switch (AuthMenu::intToAuth(authServer)) {
                    default:
                    case AuthMethod::None: {
                        loadLayer->finished();
                        FLAlertLayer::create("Error", "Unsupported <cy>authentication method</c>.", "OK")->show();
                        break;
                    }
                    case AuthMethod::DashAuth: {
#ifdef DASHAUTH
                        DashAuthRequest().getToken(Mod::get(), DASHEND_URL)->except([loadLayer](std::string err) {
                            log::warn("failed to get token :c reason: {}", err);
                            loadLayer->finished();
                            FLAlertLayer::create("DashAuth Error", "Failed to get token, view logs for reason.", "OK")->show();
                        })->then([loadLayer](std::string const& token) {
                            AuthMenu::genAuthToken(AuthMethod::DashAuth, token, false, [loadLayer](int value) {
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
#else
                        loadLayer->finished();
                        FLAlertLayer::create("Error", "Unsupported <cy>authentication method</c>.", "OK")->show();
#endif
                        break;
                    }
                    case AuthMethod::Custom: {
                        loadLayer->finished();
                        FLAlertLayer::create("Error", "Either the token you set is <cy>expired</c>, or you <cy>entered the token incorrectly!</c>", "OK")->show();
                        break;
                    }
                    case AuthMethod::Argon: {
#ifdef ARGON
                        m_fields->m_argonListener.cancel();
                        m_fields->m_argonListener.spawn(
                            argon::startAuth(),
                            [loadLayer](Result<std::string> res) {
                                if (res.isErr()) {
                                    log::warn("Argon auth failed: {}", res.unwrapErr());
                                    loadLayer->finished();
                                    FLAlertLayer::create("Argon Error", "Failed to get token, view logs for reason.", "OK")->show();
                                } else {
                                    auto token = std::move(res).unwrap();
                                    AuthMenu::genAuthToken(AuthMethod::Argon, token, false, [loadLayer](int value) {
                                        loadLayer->finished();
                                        if (value == 1) {
                                            ObjectWorkshop::create(true)->show();
                                        } else if (value == -1) {
                                            FLAlertLayer::create("Error", "Currently, Object Workshop <cy>servers are down</c> at the moment! View your logs, or view announcements on the <cy>Discord Server</c> for more information, or if there are no announcements, inform the developer of this error!", "OK")->show();
                                        } else {
                                            FLAlertLayer::create("Error", "Something went wrong when <cy>trying to generate a new authentication token!</c>\nIf this issue happens again, please consider <cr>resetting your settings</c> to redo the authentication process.", "OK")->show();
                                        }
                                    });
                                }
                            }
                        );
#else
                        loadLayer->finished();
                        FLAlertLayer::create("Error", "Unsupported <cy>authentication method</c>.", "OK")->show();
#endif
                        break;
                    }
                }
            }
        });
    } else {
        AuthMenu::create()->show();
    }
}

bool CustomObjects::init(LevelEditorLayer* editorLayer) {
    if (!EditorUI::init(editorLayer)) return false;
    alpha::editor_tabs::addTab("workshop"_spr, alpha::editor_tabs::BUILD, [this] { // Create the tab
        std::vector<Ref<CCNode>> nodes;
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
            web::WebRequest req = web::WebRequest();
            auto myjson = matjson::Value();
            myjson.set("token", token);
            req.header("Content-Type", "application/json");
            req.userAgent(USER_AGENT);
            auto certValid = Mod::get()->getSettingValue<bool>("cert-valid");
            if (!certValid) {
                req.certVerification(certValid);
            }
            req.bodyJSON(myjson);
            m_fields->m_listener.spawn(req.post(fmt::format("{}/user/@me", HOST_URL)), [this, labelOther1](web::WebResponse value) {
                if (value.json().isErr()) {
                    log::error("Invalid server response. {}", value.json().err());
                    Notification::create("Object Workshop server gave invalid response.", NotificationIcon::Warning)->show();
                    return;
                }
                auto jsonRes = value.json().unwrapOrDefault();
                if (!jsonRes.isObject()) return log::error("Response isn't object.");
                auto isError = jsonRes.get("error");
                if (isError.isOk()) return;
                auto uploadRes = jsonRes.get("uploads");

                if (uploadRes.isOk()) {
                    auto uploads = uploadRes.unwrap().asInt().unwrapOrDefault();
                    labelOther1->setString(
                        fmt::format(
                            "<cg>{}</c> Upload{} from you",
                            uploads,
                            (uploads == 1) ? "" : "s"
                        )
                    );
                }
            });
        }
        auto winSize = CCDirector::sharedDirector()->getWinSize();
        //m_fields->menu->setPosition({winSize.width / 2, 45});
        //return alpha::editor_tabs::createEditButtonBar(m_fields->menu);
        m_fields->menu->setContentSize({300, 90});
        m_fields->menu->updateLayout();
        nodes.push_back(m_fields->menu);
        return alpha::editor_tabs::createEditButtonBar(nodes);
    }, [] { // create the tab icon
        auto folder = CCSprite::createWithSpriteFrameName("gj_folderBtn_001.png");
        folder->setScale(0.4F);
        auto labelC = CCLabelBMFont::create("C+", "bigFont.fnt");
        labelC->setScale(0.4F);
        folder->addChildAtPosition(labelC, Anchor::Center);
        CCLabelBMFont* textLabelOn = CCLabelBMFont::create("C+", "bigFont.fnt");
        textLabelOn->setScale(0.4f);
        CCLabelBMFont* textLabelOff = CCLabelBMFont::create("C+", "bigFont.fnt");
        textLabelOff->setScale(0.4f);
        return folder;
    }, [] (bool state, auto tab) { // do something when the tab is entered and exited
        log::info("rawr-tab state: {}", state);
    }, [] (int rows, int cols, auto tab) { // do something when the tab is reloaded
        log::info("rawr tab rows: {}, cols: {}", rows, cols);
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

#if 0
#include <Geode/modify/ProfilePage.hpp>

class $modify(ProfilePage) {
    struct Fields {
        CCMenuItemSpriteExtra* customObjsBtn;
        async::TaskHolder<geode::utils::web::WebResponse> m_profileListener;
    };
    void onClose(CCObject* sender) {
        m_fields->m_profileListener.getFilter().cancel();
        ProfilePage::onClose(sender);
    }
    void loadPageFromUserInfo(GJUserScore* user) {
        ProfilePage::loadPageFromUserInfo(user);
        if (GJBaseGameLayer::get()) return;
        m_fields->m_profileListener.bind([this] (web::WebTask::Event* e) {
            if (web::WebResponse* value = e->getValue()) {
                log::info("Request was finished!");
                if (value->json().err() && !value->ok() && value->code() >= 500) {
                    log::error("Couldn't get server. {}", value->json().err());
                    return;
                }
                if (value->code() != 200) {
                    //log::error("Couldn't get user: {}", value->json().unwrapOrDefault().dump());
                    return;
                }
                auto jsonRes = value->json().unwrapOrDefault();
                if (!jsonRes.isObject()) return log::error("Response isn't object.");
                auto isError = jsonRes.get("error");
                if (isError.isOk()) return;
                if (m_fields->customObjsBtn == nullptr) {
                    if (auto menu = typeinfo_cast<CCMenu*>(m_mainLayer->getChildByID("socials-menu"))) {
                        m_fields->customObjsBtn = CCMenuItemExt::createSpriteExtraWithFilename("profileObjBtn.png"_spr, 0.75F,
                        [this](CCObject*) {
                            int authServerL = Mod::get()->getSettingValue<int64_t>("auth-server");
                            if (authServerL != -1) {
                                auto loadLayer = AuthLoadLayer::create();
                                loadLayer->show();
                                auto token = Mod::get()->getSettingValue<std::string>("token");
                                AuthMenu::testAuth(token, [this, loadLayer, authServerL, token](int value) {
                                    int authServer = authServerL;
                                    if (value == 1) {
                                        loadLayer->finished();
                                        ObjectWorkshop::createToUser(true, m_accountID)->show();
                                    } else if (value == -1) {
                                        loadLayer->finished();
                                        FLAlertLayer::create("Error", "Currently, Object Workshop <cy>servers are down</c> at the moment! View your logs, or view announcements on the <cy>Discord Server</c> for more information, or if there are no announcements, inform the developer of this error!", "OK")->show();
                                    } else {
                                        #if !defined(ARGON) && defined(DASHAUTH)
                                        authServer = 0; // fallback
                                        log::info("Argon disabled, fallback to DashAuth");
                                        #endif
                                        switch (AuthMenu::intToAuth(authServer)) {
                                            default:
                                            case AuthMethod::None: {
                                                loadLayer->finished();
                                                FLAlertLayer::create("Error", "Unsupported <cy>authentication method</c>.", "OK")->show();
                                                break;
                                            }
                                            case AuthMethod::DashAuth: {
                                                #ifdef DASHAUTH
                                                DashAuthRequest().getToken(Mod::get(), DASHEND_URL)->except([loadLayer](std::string err) {
                                                    log::warn("failed to get token :c reason: {}", err);
                                                    loadLayer->finished();
                                                    FLAlertLayer::create("DashAuth Error", "Failed to get token, view logs for reason.", "OK")->show();
                                                })->then([this, loadLayer](std::string const& token) {
                                                    AuthMenu::genAuthToken(AuthMethod::DashAuth, token, false, [this, loadLayer](int value) {
                                                        loadLayer->finished();
                                                        if (value == 1) {
                                                            ObjectWorkshop::createToUser(true, m_accountID)->show();
                                                        } else if (value == -1) {
                                                            FLAlertLayer::create("Error", "Currently, Object Workshop <cy>servers are down</c> at the moment! View your logs, or view announcements on the <cy>Discord Server</c> for more information, or if there are no announcements, inform the developer of this error!", "OK")->show();
                                                        } else {
                                                            FLAlertLayer::create("Error", "Something went wrong when <cy>trying to generate a new authentication token!</c>\nIf this issue happens again, please consider <cr>resetting your settings</c> to redo the authentication process.", "OK")->show();
                                                        }
                                                    });
                                                });
#else
                                                loadLayer->finished();
                                                FLAlertLayer::create("Error", "Unsupported <cy>authentication method</c>.", "OK")->show();
#endif
                                            }
                                            case AuthMethod::Custom: {
                                                loadLayer->finished();
                                                FLAlertLayer::create("Error", "Either the token you set is <cy>expired</c>, or you <cy>entered the token incorrectly!</c>", "OK")->show();
                                                break;
                                            }
                                            case AuthMethod::Argon: {
                                                #ifdef ARGON
                                                auto res = argon::startAuth([this, loadLayer](Result<std::string> res) {
                                                    if (!res) {
                                                        log::warn("Argon auth failed: {}", res.unwrapErr());
                                                        loadLayer->finished();
                                                        FLAlertLayer::create("Argon Error", "Failed to get token, view logs for reason.", "OK")->show();
                                                        return;
                                                    }
                                                    auto token = std::move(res).unwrap();
                                                    AuthMenu::genAuthToken(AuthMethod::Argon, token, false, [this, loadLayer](int value) {
                                                        loadLayer->finished();
                                                        if (value == 1) {
                                                            ObjectWorkshop::createToUser(true, m_accountID)->show();
                                                        } else if (value == -1) {
                                                            FLAlertLayer::create("Error", "Currently, Object Workshop <cy>servers are down</c> at the moment! View your logs, or view announcements on the <cy>Discord Server</c> for more information, or if there are no announcements, inform the developer of this error!", "OK")->show();
                                                        } else {
                                                            FLAlertLayer::create("Error", "Something went wrong when <cy>trying to generate a new authentication token!</c>\nIf this issue happens again, please consider <cr>resetting your settings</c> to redo the authentication process.", "OK")->show();
                                                        }
                                                    });
                                                });
                                                if (!res) {
                                                    log::warn("Failed to start auth attempt: {}", res.unwrapErr());
                                                    loadLayer->finished();
                                                    FLAlertLayer::create("Argon Error", "Failed to start auth attempt, view logs for reason.", "OK")->show();
                                                }
                                                #else
                                                loadLayer->finished();
                                                FLAlertLayer::create("Error", "Unsupported <cy>authentication method</c>.", "OK")->show();
                                                #endif
                                                break;
                                            }
                                        }
                                    }
                                });
                            } else {
                                ObjectWorkshop::createToUser(false, m_accountID)->show();
                            }
                        });
                        menu->addChild(m_fields->customObjsBtn);
                        menu->updateLayout();
                }
                }
            } else if (web::WebProgress* progress = e->getProgress()) {
                // The request is still in progress...
            } else if (e->isCancelled()) {
                log::error("Request was cancelled.");
            } else {
                log::error("what happened?");
            }
        });
        if (auto scene = CCScene::get()) {
            auto workshop = typeinfo_cast<ObjectWorkshop*>(scene->getChildByID("objectworkshop"_spr));
            if (workshop != nullptr) return; // so you cant loop lol
            web::WebRequest req = web::WebRequest();
            m_fields->m_profileListener.getFilter().cancel();
            req.userAgent(USER_AGENT);
            auto certValid = Mod::get()->getSettingValue<bool>("cert-valid");
            if (!certValid) {
                req.certVerification(certValid);
            }
            m_fields->m_profileListener.setFilter(req.get(fmt::format("{}/user/{}", HOST_URL, m_accountID)));
        }
    }
};
#endif

#include <Geode/modify/EditorUI.hpp>
class $modify(EditorUI) {
    void keyDown(enumKeyCodes key, double timestamp) {
        if (auto scene = CCScene::get()) {
            if (typeinfo_cast<ObjectWorkshop*>(scene->getChildByID("objectworkshop"_spr))) return;
        }
        EditorUI::keyDown(key, timestamp);
    }
};
