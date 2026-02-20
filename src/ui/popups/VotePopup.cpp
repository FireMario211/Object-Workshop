#include "VotePopup.hpp"
#include <UIBuilder.hpp>

bool VotePopup::init(std::string title_str, std::function<void(bool)> callback) {
    if (!Popup::init(200.f, 115.f)) return false;
    m_callback = callback;
    Build<CCLabelBMFont>::create(title_str.c_str(), "bigFont.fnt").parentAtPos(m_mainLayer, Anchor::Top, {0, -20});
    Build<CCSprite>::createSpriteName("GJ_likeBtn_001.png").scale(1.2f).intoMenuItem([this]() {
        if (m_showWarning) {
            geode::createQuickPopup(
            "Warning",
            m_warningMessage1,
            "No",
            "Yes",
            [this](auto, bool btn2) {
                if (btn2)
                    onVote(true);
            });
        } else {
            onVote(true);
        }
    }).parentAtPos(m_buttonMenu, Anchor::Center, {-43, -11});
    Build<CCSprite>::createSpriteName("GJ_dislikeBtn_001.png").scale(1.2f).intoMenuItem([this]() {
        if (m_showWarning) {
            geode::createQuickPopup(
            "Warning",
            m_warningMessage2,
            "No",
            "Yes",
            [this](auto, bool btn2) {
                if (btn2)
                    onVote(false);
            });
        } else {
            onVote(false);
        }
    }).parentAtPos(m_buttonMenu, Anchor::Center, {43, -11});
    return true;
}

void VotePopup::setWarning(std::string message1, std::string message2) {
    m_warningMessage1 = message1;
    m_warningMessage2 = message2;
    m_showWarning = true;
}

void VotePopup::onVote(bool vote) {
    m_callback(vote);
    this->onClose(nullptr);
}
