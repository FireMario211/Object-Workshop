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
        ButtonSprite::create("Argon"),
        this,
        menu_selector(AuthMenu::onArgon)
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
        if (method == AuthMethod::Argon) {
            myjson.set("username", fmt::format("{}", GJAccountManager::get()->m_username));
            myjson.set("account_id", GJAccountManager::get()->m_accountID);
        }
        req.bodyJSON(myjson);
        req.userAgent(USER_AGENT);
        switch (method) {
            case AuthMethod::Argon:
                m_authListener.setFilter(req.post(fmt::format("{}/argon", HOST_URL)));
                break;
            case AuthMethod::DashAuth:
                m_authListener.setFilter(req.post(fmt::format("{}/dashauth", HOST_URL)));
                break;
            default:
                log::warn("Unknown Auth Method, can't send request!");
                break;
        }
}
void AuthMenu::onArgon(CCObject*) {
    if (auto gjam = GJAccountManager::sharedState()) {
        if (gjam->m_accountID == 0) {
            return FLAlertLayer::create("Error", "You are not <cy>signed in</c>! The only option you can use is <cg>Do Later</c>", "OK")->show();
        }
    }
#ifdef ARGON
    geode::createQuickPopup(
        "Note",
        "Are you sure you want to use this <cy>authentication method</c>?\nThis method involves the same method used in <cy>Globed</c>, where it has your GD account <cy>send a message</c> to a <cg>bot</c>.\n\nIf this method doesn't satisfy you, consider receiving an <cy>authentication token</c> by <cy>joining the Discord server</c>\nWould you like to <cg>proceed anyways</c>?",
        "Cancel",
        "Proceed",
        [this](auto, bool btn2) {
            if (btn2) {
                this->onClose(nullptr);
                log::info("Authenticating with Argon...");
                auto res = argon::startAuth([](Result<std::string> res) {
                    if (!res) {
                        log::warn("Argon auth failed: {}", res.unwrapErr());
                        FLAlertLayer::create("Argon Error", "Failed to get token, view logs for reason.", "OK")->show();
                        return;
                    }
                    auto token = std::move(res).unwrap();
                    genAuthToken(AuthMethod::Argon, token, true, [](bool value) {
                        if (value) {
                            ObjectWorkshop::create(true)->show();
                        }
                    });
                });
                if (!res) {
                    log::warn("Failed to start auth attempt: {}", res.unwrapErr());
                    FLAlertLayer::create("Argon Error", "Failed to start auth attempt, view logs for reason.", "OK")->show();
                }
            }
        },
        true,
        true
    );
#else 
    FLAlertLayer::create("Error", "Unfortunately, this method <cr>is not available</c>.", "OK")->show();
#endif
}
void AuthMenu::onDoLater(CCObject*) {
    this->onClose(nullptr);
    Loader::get()->queueInMainThread([]() {
        ObjectWorkshop::create(false)->show();
    });
}

void AuthMenu::onInfoBtn(CCObject*) {
    //	Unfortunately, RobTop's servers <cr>IP banned my server</c>, meaning this method <cr>is not available</c>.
    FLAlertLayer::create(
        nullptr,
        "About Auth",
        "<cp>DashAuth</c> is an authentication method that <cy>sends a message to a bot</c> to confirm the authenticity of your <cy>GD account</c>, similar to how mods like <cb>Globed</c> handles verifying your <cy>GD account</c>.\n\n<cr>Argon</c> is an authentication method made by <cy>dankmeme01</c> which is similar to <cp>DashAuth</c> and is the method used by <cg>Globed</c> to authenticate..\n\n<cy>Do Later</c> is what it says. This will close the menu and open up the <cy>Object Workshop</c>. Although do note that you <cy>will be limited</c> in what features you can use.\n\nIf none of the methods satisfy you, or you would like to receive an <cy>authentication token</c> without doing any of these methods, consider <cy>joining the Discord server</c> to ask me to verify you and give an <cy>authentication token</c> to you.",
        "OK", nullptr,
        390.F,
        true,
        180.F,
        1.0F
    )->show();
}
