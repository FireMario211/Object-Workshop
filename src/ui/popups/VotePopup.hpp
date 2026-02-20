#pragma once
#include <Geode/Geode.hpp>
#include <Geode/ui/Popup.hpp>
using namespace geode::prelude;

class VotePopup : public geode::Popup {
protected:
    std::function<void(bool)> m_callback;
    std::string m_warningMessage1;
    std::string m_warningMessage2;
    bool m_showWarning = false;

    bool init(std::string, std::function<void(bool)>);
    void onVote(bool vote);
public:
    void setWarning(std::string message1, std::string message2);
    static VotePopup* create(std::string title, std::function<void(bool)> callback) {
        auto ret = new VotePopup();
        if (ret->init(title, callback)) {
            ret->autorelease();
            return ret;
        }

        delete ret;
        return nullptr;
    }
};
