#pragma once

using namespace geode::prelude;

// definitely not copied from geode
class AuthLoadLayer : public FLAlertLayer {
protected:
    LoadingCircle* m_loadingCircle;
    virtual bool init();
public:
    void finished();
    static AuthLoadLayer* create() {
        auto ret = new AuthLoadLayer();
        if (ret->init()) {
            ret->autorelease();
            return ret;
        }

        delete ret;
        return nullptr;
    }
};
