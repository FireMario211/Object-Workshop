#include "ObjectItem.hpp"

CCNode* ObjectItem::createStars(double rating) {
    auto node = CCNode::create();
    node->setLayout(
        RowLayout::create()
            ->setAxisAlignment(AxisAlignment::Center)
            ->setAutoScale(false)
            ->setCrossAxisOverflow(false)
            ->setGap(2)
            ->setGrowCrossAxis(true)
    );
    for (int i = 0; i < 5; i++) {
        double starValue = rating - i;
        CCSprite* star;
        if (starValue >= 1.0) {
            star = CCSprite::createWithSpriteFrameName("GJ_starsIcon_001.png");
        } else if (starValue >= 0.5) {
            star = CCSprite::create("halfStarsIcon.png"_spr);
        } else {
            star = CCSprite::createWithSpriteFrameName("GJ_starsIcon_gray_001.png");
        }
        star->setScale(0.45F);
        node->addChild(star);
    }
    node->setContentSize({90, 20});
    node->updateLayout();
    return node;
}

CCMenu* ObjectItem::createClickableStars(CCObject* target, SEL_MenuHandler callback) {
    auto node = CCMenu::create();
    node->setLayout(
        RowLayout::create()
            ->setAxisAlignment(AxisAlignment::Center)
            ->setAutoScale(false)
            ->setCrossAxisOverflow(false)
            ->setGap(2)
            ->setGrowCrossAxis(true)
    );
    for (int i = 0; i < 5; i++) {
        auto starNode = CCNode::create();
        CCSprite* starFull = CCSprite::createWithSpriteFrameName("GJ_starsIcon_001.png");
        CCSprite* starEmpty = CCSprite::createWithSpriteFrameName("GJ_starsIcon_gray_001.png");
        starNode->setContentSize(starFull->getContentSize());
        starNode->setAnchorPoint({0.5, 0.5});
        starFull->setVisible(false);
        starEmpty->setVisible(true);
        starFull->setAnchorPoint({0,0});
        starFull->setID("full");
        starEmpty->setAnchorPoint({0,0});
        starEmpty->setID("empty");
        starNode->addChild(starFull);
        starNode->addChild(starEmpty);
        starNode->setScale(0.45F);
        auto clickableStar = CCMenuItemSpriteExtra::create(starNode, target, callback);
        clickableStar->setID(fmt::format("{}", (i + 1)).c_str());
        node->addChild(clickableStar);
    }
    node->setContentSize({90, 20});
    node->updateLayout();
    return node;
}
bool ObjectItem::init(ObjectData data) {
    m_data = data;
    this->setContentSize({ 86.0f, 100.0f });
    
    bgSpr = cocos2d::extension::CCScale9Sprite::create("redBG.png"_spr);
    bgSpr->setContentSize({ 86.0f, 100.0f });

    auto title = CCLabelBMFont::create(data.name.c_str(), "bigFont.fnt");
    title->limitLabelWidth(45.0F, 0.8F, 0.25F); // 0.425
    auto author = CCLabelBMFont::create(fmt::format("By {}", data.authorName).c_str(), "goldFont.fnt");
    author->limitLabelWidth(45.0F, 0.7F, 0.2F); // 0.4

    auto previewBG = CCScale9Sprite::create("square02_small.png");
    previewBG->setOpacity(60);
    previewBG->setContentSize({ 72.F, 41.F });
    
    bgSpr->addChildAtPosition(title, Anchor::Center, {1, -8}); // 0.425
    bgSpr->addChildAtPosition(author, Anchor::Center, {1, -20}); // 0.4

    bgSpr->addChildAtPosition(createStars(data.rating), Anchor::BottomLeft, { -2, 5 });
    this->addChildAtPosition(bgSpr, Anchor::Center);

    CCLayerColor* mask = CCLayerColor::create({255, 255, 255});
    mask->setContentSize(previewBG->getContentSize());
    m_clippingNode = CCClippingNode::create();
    m_clippingNode->setContentSize(previewBG->getContentSize());
    m_clippingNode->setAnchorPoint({0.5, 0.5});
    if (auto editorUI = EditorUI::get() && data.objectString.length() > 0) {
        // i love ghidra (i have no idea what params are what)
        /*
        gd::string - object string, duh 
        bool - something with CCPoint
        bool - something with centering and deleting smart objects, 
        int - how many objects should render (0 = unlimited)
        cocos2d::CCArray* - something with smart blocks,
        cocos2d::CCArray* - i have no idea, maybe a filter of what objects to not show?
        GameObject* - something with keyframes
        */
        CCSprite* sprite = EditorUI::get()->spriteFromObjectString(data.objectString, false, false, 0, (CCArray *)0x0, (CCArray *)0x0,(GameObject *)0x0);
        sprite->setScale(((previewBG->getContentSize().height - 6) / sprite->getContentSize().height));
        m_clippingNode->addChildAtPosition(sprite, Anchor::Center);
    }
    m_clippingNode->setStencil(mask);
    //m_clippingNode->setAlphaThreshold(0.05F);
    m_clippingNode->setZOrder(1);
    bgSpr->addChildAtPosition(previewBG, Anchor::Top, { 0, -29 });
    bgSpr->addChildAtPosition(m_clippingNode, Anchor::Top, { 0, -29 });

    return true;
}

ObjectItem* ObjectItem::create(ObjectData data) {
    auto pRet = new ObjectItem();
    if (pRet) {
        if (pRet->init(data)) {
            pRet->autorelease();
            return pRet;
        }
    }
    CC_SAFE_DELETE(pRet);
    return nullptr;
};
