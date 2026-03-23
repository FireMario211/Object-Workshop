#include "ExtPreviewBG.hpp"
#include "../ui/ObjectWorkshop.hpp"
#include "../config.hpp"

void ExtPreviewBG::setNewPreview(std::string data) {
    if (m_clippingNode) {
        m_clippingNode->removeAllChildrenWithCleanup(true);
    }
    if (!data.empty()) {
        m_data = data;
        unsigned int objectCount = std::count(data.begin(), data.end(), ';');
        int renderLimit = Mod::get()->getSettingValue<int64_t>("render-objects");
        int preRender = Mod::get()->getSettingValue<int64_t>("prerender-objects");
        auto smartBlock = CCArray::create();
        if (objectCount >= preRender && Mod::get()->getSettingValue<bool>("prerender-full")) {
            auto sprite = m_editorLayer->m_editorUI->spriteFromObjectString(data, false, false, renderLimit, smartBlock, (CCArray *)0x0,(GameObject *)0x0);
            m_editorLayer->updateObjectColors(smartBlock);

            CCSize contentSize = sprite->getContentSize();
            sprite->setPosition(contentSize / 2);
            CCRenderTexture* tex = CCRenderTexture::create(contentSize.width, contentSize.height);
            tex->beginWithClear(0,0,0,0);
            sprite->visit();
            tex->end();
            objSprite = CCSprite::create();
            objSprite->setContentSize(sprite->getContentSize());
            objSprite->addChildAtPosition(tex, Anchor::Center);
        } else {
            objSprite = m_editorLayer->m_editorUI->spriteFromObjectString(data, false, false, renderLimit, smartBlock, (CCArray *)0x0,(GameObject *)0x0);
            m_editorLayer->updateObjectColors(smartBlock);
        }
        
        objSprite->setScale((m_clippingNode->getContentSize().height - 20) / objSprite->getContentSize().height);
        m_oldScale = objSprite->getScale();
        m_clippingNode->addChildAtPosition(objSprite, Anchor::Center, {0, -5});
        m_oldPos = objSprite->getPosition();
    }
}

bool ExtPreviewBG::init(LevelEditorLayer* editorLayer, std::string data, CCSize contentSize) {
    if (!CCLayer::init()) return false;
    m_editorLayer = editorLayer;
    this->setContentSize(contentSize);
    this->setAnchorPoint({0.5, 0.5});
    Build<CCScale9Sprite>::create("square02_small.png").opacity(60).contentSize(this->getContentSize()).with([](auto node) {
        Build<CCLabelBMFont>::create("Preview", "goldFont.fnt").scale(0.425F).parentAtPos(node, Anchor::Top, {0, -8});
    }).store(m_bg).parentAtPos(this, Anchor::Center);
    CCLayerColor* mask = CCLayerColor::create({255, 255, 255});
    mask->setContentSize(m_bg->getContentSize());
    m_clippingNode = CCClippingNode::create();
    m_clippingNode->setContentSize(m_bg->getContentSize());
    m_clippingNode->setAnchorPoint({0.5, 0.5});
    setNewPreview(data);
    m_clippingNode->setStencil(mask);
    m_clippingNode->setZOrder(1);
    this->addChildAtPosition(m_clippingNode, Anchor::Center);

    this->registerWithTouchDispatcher();
    this->setTouchEnabled(true);
    this->setTouchMode(kCCTouchesOneByOne);
    this->setMouseEnabled(true);
    return true;
}

bool ExtPreviewBG::ccTouchBegan(CCTouch* touch, CCEvent* event) {
    if (!CCLayer::ccTouchBegan(touch, event)) return false;
    auto touchPos = cocos2d::CCDirector::sharedDirector()->convertToGL(touch->getLocationInView());
    auto nodeTouchPos = m_bg->convertToNodeSpace(touchPos);
    auto boundingBox = m_bg->boundingBox();
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
    auto nodeTouchPos1 = m_bg->convertToNodeSpace(touchPos);
    auto nodeTouchPos2 = m_bg->convertToNodeSpace(m_touchStart);
    auto delta = nodeTouchPos1 - nodeTouchPos2;
    objSprite->setPosition(objSprite->getPosition() + delta);
    m_touchStart = touchPos;
}

void ExtPreviewBG::updateZoom(float amount) {
    float newZoom = m_currentZoom + amount;
    if (newZoom >= MIN_ZOOM && newZoom <= 25.0f) {
        float oldZoom = m_currentZoom;
        m_currentZoom += amount;
        float zoomFactor = m_currentZoom / oldZoom;
        
        auto relativePos = (m_oldPos - objSprite->getPosition()) / oldZoom;
        objSprite->setScale(m_oldScale * m_currentZoom);
        objSprite->setPosition(m_oldPos - (relativePos * m_currentZoom));
    }
};

void ExtPreviewBG::setZoom(float amount) {
    if (amount >= MIN_ZOOM && amount <= 25.0f) {
        float oldZoom = m_currentZoom;
        m_currentZoom = amount;
        float zoomFactor = m_currentZoom / oldZoom;
        
        auto relativePos = (m_oldPos - objSprite->getPosition()) / oldZoom;
        objSprite->setScale(m_oldScale * m_currentZoom);
        objSprite->setPosition(m_oldPos - (relativePos * m_currentZoom));
    }
};

void ExtPreviewBG::resetZoom() {
    m_currentZoom = 1.0F;
    objSprite->setScale(m_oldScale * m_currentZoom);
    objSprite->setPosition(m_oldPos);
};

ExtPreviewBG* ExtPreviewBG::create(LevelEditorLayer* editorLayer, std::string data, CCSize contentSize) {
    auto pRet = new ExtPreviewBG();
    if (pRet) {
        if (pRet->init(editorLayer, data, contentSize)) {
            pRet->autorelease();
            return pRet;
        }
    }
    CC_SAFE_DELETE(pRet);
    return nullptr;
};
