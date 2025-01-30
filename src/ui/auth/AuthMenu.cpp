#include "../../config.hpp"
#include "AuthMenu.hpp"
#include "../ObjectWorkshop.hpp"

EventListener<web::WebTask> m_authListener;
EventListener<web::WebTask> m_iconListener;

bool hasEmitted = false;

bool AuthMenu::setup() {
    this->setTitle("Authentication");
    auto dashAuthBtn = CCMenuItemSpriteExtra::create(
        ButtonSprite::create("DashAuth"),
        this,
        menu_selector(AuthMenu::onDashAuth)
    );
    auto gdAuthBtn = CCMenuItemSpriteExtra::create(
        ButtonSprite::create("GDAuth"),
        this,
        menu_selector(AuthMenu::onGDAuth)
    );
    auto laterSpr = ButtonSprite::create("Do Later", "bigFont.fnt", "GJ_button_01.png");
    laterSpr->setScale(0.7F);
    auto laterBtn = CCMenuItemSpriteExtra::create(
        laterSpr,
        this,
        menu_selector(AuthMenu::onDoLater)
    );
    m_buttonMenu->addChildAtPosition(dashAuthBtn, Anchor::Center, {0, 20});
    m_buttonMenu->addChildAtPosition(gdAuthBtn, Anchor::Center, {0, -15});
    m_buttonMenu->addChildAtPosition(laterBtn, Anchor::Bottom, {0, 20});

    auto infoSpr = CCSprite::createWithSpriteFrameName("GJ_infoIcon_001.png");
    infoSpr->setScale(1.25F);
    auto infoBtn = CCMenuItemSpriteExtra::create(
        infoSpr,
        this,
        menu_selector(AuthMenu::onInfoBtn)
    );
    m_buttonMenu->addChildAtPosition(infoBtn, Anchor::TopRight);
    return true;
}


/*
geode::createQuickPopup(
        "Warning",
        "Are you sure you want to use this <cy>authentication method</c>?\nThis method involves <cy>sending your GJP</c> (GD password) to <cg>fig's server</c> (third party), meaning you are <cr>trusting this server to not use your GD account with malice</c>.\n\nIf this method doesn't satisfy you, consider receiving an <cy>authentication token</c> by <cy>joining the Discord server</c>\nWould you like to <cg>proceed anyways</c>?",
        "Cancel",
        "Proceed",
        [this](auto, bool btn2) {
*/

void AuthMenu::onDashAuth(CCObject*) {
    if (auto gjam = GJAccountManager::sharedState()) {
        if (gjam->m_accountID == 0) {
            return FLAlertLayer::create("Error", "You are not <cy>signed in</c>! The only option you can use is <cg>Do Later</c>", "OK")->show();
        }
    }
#ifdef DASHAUTH
    geode::createQuickPopup(
        "Note",
        "Are you sure you want to use this <cy>authentication method</c>?\nThis method involves having your GD account <cy>send a message</c> to <cg>GDAuth</c>.\n\nIf this method doesn't satisfy you, consider receiving an <cy>authentication token</c> by <cy>joining the Discord server</c>\nWould you like to <cg>proceed anyways</c>?",
        "Cancel",
        "Proceed",
        [this](auto, bool btn2) {
            if (btn2) {
                this->onClose(nullptr);
                log::info("Authenticating with DashAuth...");
                DashAuthRequest().getToken(Mod::get(), DASHEND_URL)->except([](std::string err) {
                    log::warn("failed to get token :c reason: {}", err);
                    FLAlertLayer::create("DashAuth Error", "Failed to get token, this could be due to the message expiring too early. <cy>Please try again</c>, or check logs to view a detailed reason.", "OK")->show();
                })->then([](std::string const& token) {
                    log::info("got token!! {} :3", token);
                    genAuthToken(AuthMethod::DashAuth, token, true, [](bool value) {
                        if (value) {
                            ObjectWorkshop::create(true)->show();
                        }
                    });
                });
            }
        },
        true,
        true
    );
#else
    FLAlertLayer::create("Error", "Unfortunately, RobTop's servers <cr>IP banned my server</c>, meaning this method <cr>is not available</c>.", "OK")->show();
#endif
}

void AuthMenu::testAuth(std::string token, std::function<void(int)> callback) {
    m_iconListener.bind([] (web::WebTask::Event* e) {
        if (web::WebResponse* value = e->getValue()) {
            hasEmitted = true;
            log::info("Updated Icon");
        }
    });
    m_authListener.bind([callback, token] (web::WebTask::Event* e) {
        if (web::WebResponse* value = e->getValue()) {
            if (value->json().isErr() && !value->ok() && value->code() >= 500) {
                std::string err = value->string().unwrapOrDefault();
                log::error("Couldn't get server. {}", err);
                callback(-1);
                return;
            }
            log::info("Request was finished!");
            auto jsonRes = value->json().unwrapOrDefault();
            if (jsonRes.isObject()) {
                auto isError = jsonRes.get("error");
                if (isError.isOk()) {
                    log::error("{}", jsonRes.dump());
                    callback(0);
                } else {
                    if (!hasEmitted) {
                        web::WebRequest req = web::WebRequest();
                        req.userAgent(USER_AGENT);
                        if (auto gm = GameManager::sharedState()) {
                            auto myjson = matjson::Value();
                            std::vector<matjson::Value> iconSet;
                            iconSet.push_back(gm->getPlayerFrame());
                            iconSet.push_back(gm->getPlayerColor());
                            iconSet.push_back(gm->getPlayerColor2());
                            iconSet.push_back(gm->getPlayerGlowColor());
                            iconSet.push_back(static_cast<int>(gm->getPlayerGlow()));
                            myjson.set("token", token);
                            myjson.set("icon", iconSet);
                            req.header("Content-Type", "application/json");
                            req.bodyJSON(myjson);
                            m_iconListener.setFilter(req.post(fmt::format("{}/icon", HOST_URL)));
                        }
                    }
                    log::info("The token provided is valid!");
                    callback(1);
                }
            } else {
                auto strValue = value->string().unwrapOrDefault();
                log::error("Got value {}, expected valid JSON.", strValue);
                callback(0);
            }
        } else if (web::WebProgress* progress = e->getProgress()) {
            // The request is still in progress...
        } else if (e->isCancelled()) {
            log::error("Request was cancelled.");
            callback(0);
        } else {
            log::error("what happened?");
            callback(0);
        }
    });
    web::WebRequest req = web::WebRequest();
    m_authListener.getFilter().cancel();
    auto myjson = matjson::Value();
    myjson.set("token", token);
    req.header("Content-Type", "application/json");
    req.bodyJSON(myjson);
    req.userAgent(USER_AGENT);
    m_authListener.setFilter(req.post(fmt::format("{}/verify", HOST_URL)));
}
void AuthMenu::genAuthToken(AuthMethod method, std::string token, bool showFLAlert, std::function<void(int)> callback) {
    m_authListener.bind([method, showFLAlert, callback] (web::WebTask::Event* e) {
        if (web::WebResponse* value = e->getValue()) {
            log::info("Request was finished!");
            if (value->json().isErr() && !value->ok() && value->code() >= 500) {
                std::string err = value->string().unwrapOrDefault();
                log::error("Couldn't get server. {}", err);
                callback(-1);
                return;
            }
            auto jsonRes = value->json().unwrapOrDefault();
            if (jsonRes.isObject()) {
                auto isError = jsonRes.get("error");
                if (isError.isOk()) {
                    if (showFLAlert) {
                        FLAlertLayer::create("Error", isError.unwrap().asString().unwrapOrDefault(), "OK")->show();
                    }
                    callback(0);
                } else {
                    auto token = jsonRes.get("token");
                    if (token.isOk()) {
                        Mod::get()->setSettingValue<std::string>("token", token.unwrap().asString().unwrapOrDefault());
                        Mod::get()->setSettingValue<int64_t>("auth-server", method);
                        callback(1);
                    } else {
                        log::error("Expected token, got an unknown result. {}", jsonRes.dump());
                        callback(0);
                    }
                }
            } else {
                auto strValue = value->string().unwrapOrDefault();
                log::error("Got value {}, expected valid JSON.", strValue);
                if (showFLAlert) {
                    FLAlertLayer::create("Error", "Something went wrong when trying to parse the request.", "OK")->show();
                }
                callback(0);
            }
        } else if (web::WebProgress* progress = e->getProgress()) {
            // The request is still in progress...
        } else if (e->isCancelled()) {
            log::error("Request was cancelled.");
            callback(0);
        } else {
            log::error("what happened?");
            callback(0);
        }
    });
        web::WebRequest req = web::WebRequest();
        m_authListener.getFilter().cancel();
        req.header("Content-Type", "application/json");
        log::info("Authenticated! Now sending token to backend...");
        auto myjson = matjson::Value();
        myjson.set("token", token);
        req.bodyJSON(myjson);
        req.userAgent(USER_AGENT);
        switch (method) {
            case AuthMethod::GDAuth:
                m_authListener.setFilter(req.post(fmt::format("{}/gdauth", HOST_URL)));
                break;
            case AuthMethod::DashAuth:
                m_authListener.setFilter(req.post(fmt::format("{}/dashauth", HOST_URL)));
                break;
            default:
                log::warn("Unknown Auth Method, can't send request!");
                break;
        }
}
void AuthMenu::onGDAuth(CCObject*) {
    if (auto gjam = GJAccountManager::sharedState()) {
        if (gjam->m_accountID == 0) {
            return FLAlertLayer::create("Error", "You are not <cy>signed in</c>! The only option you can use is <cg>Do Later</c>", "OK")->show();
        }
    }
#ifdef GDAUTH
    geode::createQuickPopup(
        "Warning",
        "Are you sure you want to use this <cy>authentication method</c>?\nThis method involves <cy>sending your GJP</c> (GD password) to <cg>fig's server</c> (third party), meaning you are <cr>trusting this server to not use your GD account with malice</c>.\n\nIf this method doesn't satisfy you, consider receiving an <cy>authentication token</c> by <cy>joining the Discord server</c>\nWould you like to <cg>proceed anyways</c>?",
        "Cancel",
        "Proceed",
        [this](auto, bool btn2) {
            if (btn2) {
                this->onClose(nullptr);
                log::info("Authenticating with GDAuth...");
                authentication::AuthenticationManager::get()->getAuthenticationToken([this](std::string token) {
                    genAuthToken(AuthMethod::GDAuth, token, true, [](bool value) {
                        if (value) {
                            ObjectWorkshop::create(true)->show();
                        }
                    });
                }); 
            }
        },
        true,
        true
    );
#else 
    FLAlertLayer::create("Error", "Unfortunately, <cy>fig</c> has not ported <cg>GDAuth</c> to <cy>2.207</c>, meaning this method <cr>is not available</c>. (please tell him to look at my pr that allows it to work for 2.207)", "OK")->show();
#endif
}
void AuthMenu::onDoLater(CCObject*) {
    this->onClose(nullptr);
    Loader::get()->queueInMainThread([]() {
        ObjectWorkshop::create(false)->show();
    });
}

void AuthMenu::onInfoBtn(CCObject*) {
    //	static FLAlertLayer* create(FLAlertLayerProtocol* delegate, char const* title, gd::string desc, char const* btn1, char const* btn2, float width, bool scroll, float height, float textScale) = win 0x50ac0, m1 0x4083e8, imac 0x4a4da0, ios 0x2bbef8;
    FLAlertLayer::create(
        nullptr,
        "About Auth",
        "<cp>DashAuth</c> is an authentication method that <cy>sends a message to a bot</c> to confirm the authenticity of your <cy>GD account</c>, similar to how mods like <cb>Globed</c> handles verifying your <cy>GD account</c>. Unfortunately, RobTop's servers <cr>IP banned my server</c>, meaning this method <cr>is not available</c>.\n\n<cg>GDAuth</c> is an authentication method made by <cg>fig</c> which <cy>sends your GJP</c> (session token for GD) to <cg>fig's server</c> to confirm the authenticity of your <cy>GD account</c>. You should only use this <cy>if you trust fig's server</c>, as this method is like sending your GD account to <cy>fig's server</c>.\n\n<cy>Do Later</c> is what it says. This will close the menu and open up the <cy>Object Workshop</c>. Although do note that you <cy>will be limited</c> in what features you can use.\n\nIf none of the methods satisfy you, or you would like to receive an <cy>authentication token</c> without doing any of these methods, consider <cy>joining the Discord server</c> to ask me to verify you and give an <cy>authentication token</c> to you.",
        "OK", nullptr,
        390.F,
        true,
        180.F,
        1.0F
    )->show();
}
