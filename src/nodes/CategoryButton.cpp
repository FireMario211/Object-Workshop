#include "CategoryButton.hpp"
#include <UIBuilder.hpp>

void CategoryButton::setIndicatorState(bool enabled) {
    m_bgSprBehind->setVisible(enabled);
}

bool CategoryButton::init(const char* sprName, bool isSpriteFrame, bool bgVisible) {
    if (!CCNode::init()) return false;
    this->setContentSize({ 27.0f, 27.0f });
    if (!bgVisible) {
        if (isSpriteFrame) {
            Build<CCSprite>::createSpriteName(sprName).with([this](auto node) {
                node->setScale((this->getContentWidth() - 3) / node->getContentWidth());
            }).parentAtPos(this, Anchor::Center);
        } else {
            Build<CCSprite>::create(sprName).with([this](auto node) {
                node->setScale((this->getContentWidth() - 3) / node->getContentWidth());
            }).parentAtPos(this, Anchor::Center);
        }
    } else {
        Build<CCScale9Sprite>::create("square02_small.png").opacity(60).contentSize(this->getContentSize()).with([sprName, isSpriteFrame](auto box) {
            // should be 0.75f
            if (isSpriteFrame) {
                Build<CCSprite>::createSpriteName(sprName).with([box](auto node) {
                    node->setScale((box->getContentWidth() - 9) / node->getContentWidth());
                }).parentAtPos(box, Anchor::Center);
            } else {
                Build<CCSprite>::create(sprName).with([box](auto node) {
                    node->setScale((box->getContentWidth() - 9) / node->getContentWidth());
                }).parentAtPos(box, Anchor::Center);
            }
        }).parentAtPos(this, Anchor::Center);
    }
    Build<CCSprite>::create("select_outline.png"_spr).scale(0.35f).visible(false).store(m_bgSprBehind).parentAtPos(this, Anchor::Center);
    return true;
}

CategoryButton* CategoryButton::create(const char* sprName, bool isSpriteFrame, bool bgVisible) {
    auto pRet = new CategoryButton();
    if (pRet) {
        if (pRet->init(sprName, isSpriteFrame, bgVisible)) {
            pRet->autorelease();
            return pRet;
        }
    }
    CC_SAFE_DELETE(pRet);
    return nullptr;
};
