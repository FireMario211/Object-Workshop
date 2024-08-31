#include "CategoryButton.hpp"
/*
bool CategoryButton::init(const char* string, SEL_MenuHandler callback) {
    auto menu = CCMenu::create();
    auto label = CCLabelBMFont::create(string, "bigFont.fnt");
    label->limitLabelWidth(200.0F, 0.75F, 0.2F);
    
    auto bgNode = CCNode::create();
    auto bgSpr = cocos2d::extension::CCScale9Sprite::create("square02b_001.png");
    bgSpr->setColor({74, 40, 25});
    bgSpr->setContentSize({ 175.0f, 35.0f });
    bgSpr->setScale(0.5F);
    auto bgSprBehind = cocos2d::extension::CCScale9Sprite::create("square02b_001.png");
    bgSprBehind->setColor({62, 35, 20});
    bgSprBehind->setContentSize({ 180.0f, 40.0f });
    bgSprBehind->setScale(0.5F);

    bgSpr->addChildAtPosition(label, Anchor::Center, { 10, 0 });

    auto activeSideIndicator = cocos2d::extension::CCScale9Sprite::create("GJ_button_02.png");
    activeSideIndicator->setScale(0.8F);
    activeSideIndicator->setContentSize({28, 50});
    activeSideIndicator->setAnchorPoint({0.1, 0.5});

    auto inactiveSideIndicator = cocos2d::extension::CCScale9Sprite::create("GJ_button_01.png");
    inactiveSideIndicator->setScale(0.8F);
    inactiveSideIndicator->setContentSize({20, 50});
    inactiveSideIndicator->setAnchorPoint({0.1, 0.5});

    bgSpr->addChildAtPosition(inactiveSideIndicator, Anchor::Left);
    bgNode->addChildAtPosition(bgSprBehind, Anchor::Center);
    bgNode->addChildAtPosition(bgSpr, Anchor::Center);

    auto btn = CCMenuItemSpriteExtra::create(
        bgNode,
        this,
        callback
    );
    menu->addChild(btn);
    menu->setPosition({0,0});
    this->addChild(menu);
    menu->registerWithTouchDispatcher();
    menu->setTouchEnabled(true);
    cocos::handleTouchPriority(menu);
    return true;
}
*/

void CategoryButton::setIndicatorState(bool enabled) {
    activeSideIndicator->setVisible(enabled);
    inactiveSideIndicator->setVisible(!enabled);

    // i spent so much time
    activeSideIndicator->setVisible(false);
    inactiveSideIndicator->setVisible(false);
    if (enabled) {
        bgSpr->setColor({63, 102, 92});
        bgSprBehind->setColor({27, 142, 143});
    } else {
        bgSpr->setColor({86, 48, 29});
        bgSprBehind->setColor({62, 35, 20});
    }
}

bool CategoryButton::init(const char* string) {
    auto label = CCLabelBMFont::create(string, "bigFont.fnt");
    label->limitLabelWidth(150.0F, 0.75F, 0.2F);
    
    //auto bgNode = cocos2d::extension::CCScale9Sprite::create("square02b_001.png");
    //bgNode->setOpacity(0);
    //bgNode->setContentSize({ 90.0f, 20.0f });
    this->setContentSize({ 90.0f, 20.0f });

    bgSpr = cocos2d::extension::CCScale9Sprite::create("square02b_001.png");
    bgSpr->setColor({86, 48, 29});
    bgSpr->setContentSize({ 175.0f, 35.0f });
    bgSpr->setScale(0.5F);
    bgSprBehind = cocos2d::extension::CCScale9Sprite::create("square02b_001.png");
    bgSprBehind->setColor({62, 35, 20});
    bgSprBehind->setContentSize({ 180.0f, 40.0f });
    bgSprBehind->setScale(0.5F);

    ///bgSpr->addChildAtPosition(label, Anchor::Center, { 10, 0 });
    bgSpr->addChildAtPosition(label, Anchor::Center);

    activeSideIndicator = cocos2d::extension::CCScale9Sprite::create("GJ_button_02.png");
    activeSideIndicator->setScale(0.8F);
    activeSideIndicator->setContentSize({28, 50});
    activeSideIndicator->setAnchorPoint({0.1, 0.5});
    activeSideIndicator->setVisible(false);

    inactiveSideIndicator = cocos2d::extension::CCScale9Sprite::create("GJ_button_01.png");
    inactiveSideIndicator->setScale(0.8F);
    inactiveSideIndicator->setContentSize({20, 50});
    inactiveSideIndicator->setAnchorPoint({0.1, 0.5});

    inactiveSideIndicator->setVisible(false);
    activeSideIndicator->setVisible(false);
    bgSpr->addChildAtPosition(inactiveSideIndicator, Anchor::Left);
    bgSpr->addChildAtPosition(activeSideIndicator, Anchor::Left);
    this->addChildAtPosition(bgSprBehind, Anchor::Center);
    this->addChildAtPosition(bgSpr, Anchor::Center);
    return true;
}

CategoryButton* CategoryButton::create(const char* string) {
    auto pRet = new CategoryButton();
    if (pRet) {
        if (pRet->init(string)) {
            pRet->autorelease();
            return pRet;
        }
    }
    CC_SAFE_DELETE(pRet);
    return nullptr;
};
