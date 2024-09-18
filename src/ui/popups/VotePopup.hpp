#pragma once
#include <Geode/Geode.hpp>
#include <Geode/ui/Popup.hpp>
using namespace geode::prelude;

class VotePopup : public geode::Popup<std::string, utils::MiniFunction<void(bool)>> {
protected:
    utils::MiniFunction<void(bool)> m_callback;
    std::string m_warningMessage1;
    std::string m_warningMessage2;
    bool m_showWarning = false;

    bool setup(std::string, utils::MiniFunction<void(bool)>) override;
    void onLike(CCObject*);
    void onDislike(CCObject*);
    void onVote(bool vote);
public:
    void setWarning(std::string message1, std::string message2);
    static VotePopup* create(std::string title, utils::MiniFunction<void(bool)> callback) {
        auto ret = new VotePopup();
        if (ret->initAnchored(200.f, 115.f, title, callback)) {
            ret->autorelease();
            return ret;
        }

        delete ret;
        return nullptr;
    }
};
