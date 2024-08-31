#pragma once
#include <Geode/Geode.hpp>

using namespace geode::prelude;

enum AuthMethod {
    None = -1,
    DashAuth = 0,
    GDAuth = 1
};

class AuthMenu : public geode::Popup<> {
protected:
    bool setup() override;
    void onInfoBtn(CCObject*);
    void onDashAuth(CCObject*);
    void onGDAuth(CCObject*);
    void onDoLater(CCObject*);
public:
    static void genAuthToken(AuthMethod method, std::string token, bool showFLAlert, utils::MiniFunction<void(bool)> callback);
    static void testAuth(std::string token, utils::MiniFunction<void(bool)> callback);
    static AuthMethod intToAuth(int method) {
        switch (method) {
            case -1:
            default:
                return AuthMethod::None;
            case 0:
                return AuthMethod::DashAuth;
            case 1:
                return AuthMethod::GDAuth;
        }
    }
    static AuthMenu* create() {
        auto ret = new AuthMenu();
        if (ret->initAnchored(180.f, 140.f)) {
            ret->autorelease();
            return ret;
        }
        delete ret;
        return nullptr;
    }
};