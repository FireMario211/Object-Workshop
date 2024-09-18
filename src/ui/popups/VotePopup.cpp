#include "VotePopup.hpp"

bool VotePopup::setup(std::string title_str, utils::MiniFunction<void(bool)> callback) {
    m_callback = callback;
    auto title = CCLabelBMFont::create(title_str.c_str(), "bigFont.fnt");
    m_mainLayer->addChildAtPosition(title, Anchor::Top, {0, -20});
    auto likeSpr = CCSprite::createWithSpriteFrameName("GJ_likeBtn_001.png");
    likeSpr->setScale(1.2F);
    auto likeBtn = CCMenuItemSpriteExtra::create(
        likeSpr, this, menu_selector(VotePopup::onLike)
    );
    auto dislikeSpr = CCSprite::createWithSpriteFrameName("GJ_dislikeBtn_001.png");
    dislikeSpr->setScale(1.2F);
    auto dislikeBtn = CCMenuItemSpriteExtra::create(
        dislikeSpr, this, menu_selector(VotePopup::onDislike)
    );
    m_buttonMenu->addChildAtPosition(likeBtn, Anchor::Center, {-43, -11});
    m_buttonMenu->addChildAtPosition(dislikeBtn, Anchor::Center, {43, -11});
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

void VotePopup::onLike(CCObject*) {
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
}
void VotePopup::onDislike(CCObject*) {
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
}
