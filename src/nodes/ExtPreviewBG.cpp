#include "ExtPreviewBG.hpp"
bool ExtPreviewBG::init(std::string data) {
    if (!CCLayer::init()) return false;
    this->setContentSize({ 124.F, 82.F });
    this->setAnchorPoint({0.5, 0.5});
    bg = CCScale9Sprite::create("square02_small.png");
    bg->setOpacity(60);
    bg->setContentSize({ 124.F, 82.F });

    auto previewLabel = CCLabelBMFont::create("Preview", "goldFont.fnt");
    previewLabel->setScale(0.425F);
    bg->addChildAtPosition(previewLabel, Anchor::Top, {0,-8});
    this->addChildAtPosition(bg, Anchor::Center);

    CCLayerColor* mask = CCLayerColor::create({255, 255, 255});
    mask->setContentSize(bg->getContentSize());
    auto clippingNode = CCClippingNode::create();
    clippingNode->setContentSize(bg->getContentSize());
    clippingNode->setAnchorPoint({0.5, 0.5});
    if (auto editorUI = EditorUI::get() && data.length() > 0) {
        auto renderLimit = Mod::get()->getSettingValue<int64_t>("render-objects");
        auto smartBlock = CCArray::create();
        objSprite = EditorUI::get()->spriteFromObjectString(data, false, false, renderLimit, smartBlock, (CCArray *)0x0,(GameObject *)0x0);
        LevelEditorLayer::get()->updateObjectColors(smartBlock);
        objSprite->setScale((clippingNode->getContentSize().height - 20) / objSprite->getContentSize().height);
        m_oldScale = objSprite->getScale();
        clippingNode->addChildAtPosition(objSprite, Anchor::Center, {0, -5});
    }
    clippingNode->setStencil(mask);
    clippingNode->setZOrder(1);
    this->addChildAtPosition(clippingNode, Anchor::Center);

    this->registerWithTouchDispatcher();
    this->setTouchEnabled(true);
    this->setTouchMode(kCCTouchesOneByOne);
    this->setMouseEnabled(true);
    return true;
}

bool ExtPreviewBG::ccTouchBegan(CCTouch* touch, CCEvent* event) {
    if (!CCLayer::ccTouchBegan(touch, event)) return false;
    auto touchPos = cocos2d::CCDirector::sharedDirector()->convertToGL(touch->getLocationInView());
    auto nodeTouchPos = bg->convertToNodeSpace(touchPos);
    auto boundingBox = bg->boundingBox();
    int offset = 16;
    auto newBoundingBox = CCRect(
        boundingBox.getMinX() + offset,
        boundingBox.getMinY() + offset,  
        boundingBox.getMaxX() - offset,
        boundingBox.getMaxY() - offset
    );
    newBoundingBox.size = newBoundingBox.size / 1.25F;
    m_touchStart = touchPos;
    
    bool allowTouch = newBoundingBox.intersectsRect({ nodeTouchPos.x, nodeTouchPos.y, nodeTouchPos.x, nodeTouchPos.y });
    // the amount of pain i had to code this
    return allowTouch;
}
void ExtPreviewBG::ccTouchMoved(CCTouch* touch, CCEvent* event) {
    CCLayer::ccTouchMoved(touch, event);
    auto touchPos = cocos2d::CCDirector::sharedDirector()->convertToGL(touch->getLocationInView());
    auto nodeTouchPos1 = bg->convertToNodeSpace(touchPos);
    auto nodeTouchPos2 = bg->convertToNodeSpace(m_touchStart);
    auto delta = nodeTouchPos1 - nodeTouchPos2;
    objSprite->setPosition(objSprite->getPosition() + delta);
    m_touchStart = touchPos;
}

void ExtPreviewBG::updateZoom(float amount) {
    if (!(m_currentZoom < 0) && m_currentZoom < 25) m_currentZoom += amount;
    objSprite->setScale(m_oldScale * m_currentZoom);
};
void ExtPreviewBG::resetZoom() {
    m_currentZoom = 1.0F;
    objSprite->setScale(m_oldScale * m_currentZoom);
};

ExtPreviewBG* ExtPreviewBG::create(std::string data) {
    auto pRet = new ExtPreviewBG();
    if (pRet) {
        if (pRet->init(data)) {
            pRet->autorelease();
            return pRet;
        }
    }
    CC_SAFE_DELETE(pRet);
    return nullptr;
};
