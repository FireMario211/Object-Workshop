#include "CaseCell.hpp"
#include "../config.hpp"
#include "../utils.hpp"

bool CaseCell::init(UserData user, CaseData data, std::function<void()> callback) {
    if (!CCScale9Sprite::init()) return false;
    m_user = user;
    m_case = data;
    m_forceRefresh = callback;

    this->setAnchorPoint({ 0.5, 0.5 });
    this->setContentSize({ 200.0f, 40.0f });
    this->setID("case/cell"_spr);

    auto menu = CCMenu::create();
    menu->setAnchorPoint({0.5, 0.5});
    menu->setContentSize(this->getContentSize());

    auto bgSpr = cocos2d::extension::CCScale9Sprite::create("square02b_001.png");
    bgSpr->setContentSize(this->getContentSize());
    bgSpr->setColor({95, 53, 31});

    auto authorName = CCLabelBMFont::create(data.staff_account_name.c_str(), "goldFont.fnt");
    authorName->limitLabelWidth(80.0F, 0.45F, 0.1F); // 0.425
    authorName->setAnchorPoint({0, 0.5});

    auto commentText = TextArea::create(
        data.reason, "chatFont.fnt",
        Utils::calculateScale(data.reason, 16, 100, 0.6F, 0.4F),
        150.F, { 0.0f, 0.5f }, // 1.0
        Utils::calculateScale(data.reason, 16, 100, 15.F, 7.F),
        true
    );
    commentText->setID("case/area"_spr);
	commentText->setAnchorPoint({ 0.0f, 0.5f });

    auto timeAgo = CCLabelBMFont::create(data.timestamp.c_str(), "chatFont.fnt");
    timeAgo->setColor({0,0,0});
    timeAgo->setAnchorPoint({1, 0.5});
    timeAgo->setScale(0.35F);
    timeAgo->setOpacity(125);
    timeAgo->setID("case/timestamp"_spr);
/*
    auto voteSpr = CCSprite::createWithSpriteFrameName((m_data.likes >= 0) ? "GJ_likesIcon_001.png" : "GJ_dislikesIcon_001.png");
    voteSpr->setScale(0.5F);
    auto voteBtn = CCMenuItemSpriteExtra::create(voteSpr, this, menu_selector(OWCommentCell::onVote));

    auto deleteSpr = CCSprite::createWithSpriteFrameName("GJ_deleteIcon_001.png");
    deleteSpr->setScale(0.45F);
    auto deleteBtn = CCMenuItemSpriteExtra::create(deleteSpr, this, menu_selector(OWCommentCell::onDelete));
    
    auto pinSpr = CCSprite::createWithSpriteFrameName("GJ_starsIcon_001.png");
    pinSpr->setScale(0.5F);
    auto pinBtn = CCMenuItemSpriteExtra::create(pinSpr, this, menu_selector(OWCommentCell::onPin));

    auto votesLabel = CCLabelBMFont::create(std::to_string(m_data.likes).c_str(), "bigFont.fnt");
    votesLabel->setScale(0.3F);
    votesLabel->setID("comment/votes"_spr);
*/
    auto ackBox = CCSprite::createWithSpriteFrameName((data.ack) ? "GJ_checkOn_001.png" : "GJ_checkOff_001.png");
    ackBox->setScale(0.6F);
    bgSpr->addChildAtPosition(ackBox, Anchor::Right, {-12, 0});

    bgSpr->addChildAtPosition(commentText, Anchor::Left, {5, 0});
    bgSpr->addChildAtPosition(authorName, Anchor::TopLeft, {5, -9});
    bgSpr->addChildAtPosition(timeAgo, Anchor::BottomRight, {-4, 5});
    this->addChildAtPosition(bgSpr, Anchor::Center);
    this->addChildAtPosition(menu, Anchor::Center);
    return true;
}
CaseCell* CaseCell::create(UserData user, CaseData data, std::function<void()> callback) {
    auto pRet = new CaseCell();
    if (pRet) {
        if (pRet->init(user, data, callback)) {
            pRet->autorelease();
            return pRet;
        }
    }
    CC_SAFE_DELETE(pRet);
    return nullptr;
};
