#pragma once
#include <Geode/Geode.hpp>

#ifdef GDAUTH
//#include <fig.authentication/include/authentication.hpp>
#include "../../gdauth/authentication.hpp"
#endif

//#ifdef DASHAUTH
#include <dashauth.hpp>

//#include "dashauthloll.hpp"
using namespace dashauth;
//#endif

using namespace geode::prelude;

enum AuthMethod {
    None = -1,
    DashAuth = 0,
    GDAuth = 1,
    Custom = 2
};

class AuthMenu : public geode::Popup<> {
protected:
    bool setup() override;
    void onInfoBtn(CCObject*);
    void onDashAuth(CCObject*);
    void onGDAuth(CCObject*);
    void onDoLater(CCObject*);
public:
    static void genAuthToken(AuthMethod method, std::string token, bool showFLAlert, std::function<void(int)> callback);
    static void testAuth(std::string token, std::function<void(int)> callback);
    static AuthMethod intToAuth(int method) {
        switch (method) {
            case -1:
            default:
                return AuthMethod::None;
            case 0:
                return AuthMethod::DashAuth;
            case 1:
                return AuthMethod::GDAuth;
            case 2:
                return AuthMethod::Custom;
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
