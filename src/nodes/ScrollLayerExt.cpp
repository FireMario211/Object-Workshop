#include "ScrollLayerExt.hpp"
// all thanks to https://github.com/CallocGD/GD-2.205-Decompiled
// and also from https://github.com/geode-sdk/geode

void ScrollLayerExt::scrollWheel(float pointX, float pointY) {
    if (pointX != 0.0) {
        CCScrollLayerExt::scrollLayer(pointX);
    }
}


bool ScrollLayerExt::ccTouchBegan(cocos2d::CCTouch *touch, cocos2d::CCEvent *event) {
    if (nodeIsVisible(this)) {
        bool value = CCScrollLayerExt::ccTouchBegan(touch, event);
        if (value) {
            m_touchStart = touch;
            auto touchPos = cocos2d::CCDirector::sharedDirector()->convertToGL(m_touchStart->getLocationInView());
            m_touchStartPosition2 = touchPos;
            m_touchPosition2 = touchPos;
            if (m_touchOutOfBoundary) {
                //schedule(schedule_selector(ScrollLayerExt::checkBoundaryOfContent));
                m_touchOutOfBoundary = true;
            }
            m_touchLastY = m_touchPosition2.y;
            return true;
        }
    }
    return false;
}

void ScrollLayerExt::ccTouchCancelled(cocos2d::CCTouch *touch, cocos2d::CCEvent *event) {
    if (m_cancellingTouches) return;
    if (m_touchMoved == true) {
        CCScrollLayerExt::ccTouchCancelled(touch, event);
        touchFinish(touch);
        if (m_touchOutOfBoundary != false) {
            //unschedule(schedule_selector(ScrollLayerExt::checkBoundaryOfContent));
            m_touchOutOfBoundary = false;
        }
    }
}


void ScrollLayerExt::ccTouchEnded(cocos2d::CCTouch *touch, cocos2d::CCEvent *event) {
    CCScrollLayerExt::ccTouchEnded(touch, event);
    touchFinish(touch);
    if (m_cancellingTouches != false) {
        //unschedule(schedule_selector(ScrollLayerExt::checkBoundaryOfContent));
        m_cancellingTouches = false;
    }
}

void ScrollLayerExt::ccTouchMoved(cocos2d::CCTouch *touch, cocos2d::CCEvent *event) {
    CCScrollLayerExt::ccTouchMoved(touch, event);
    m_touchMoved = true;
    auto touchPoint = cocos2d::CCDirector::sharedDirector()->convertToGL(touch->getLocationInView());
    if (touch == m_touchStart) {
        m_touchPosition2 = m_touchPosition2 - touchPoint;
    }
    if (fabsf(touchPoint.y - m_touchLastY) >= 10.F) {
        m_touchLastY = touchPoint.y;
        cancelAndStoleTouch(touch, event);
    }
    if (m_callbackMove) m_callbackMove();
}

void ScrollLayerExt::claimTouch(CCTouch* touch) {
    auto touchDispatcher = CCDirector::sharedDirector()->getTouchDispatcher();
    auto handler = (CCTargetedTouchHandler *)touchDispatcher->findHandler(this);		
    if (handler) {
        CCSet* claimedTouches = handler->getClaimedTouches();
        if (!claimedTouches->containsObject(touch)) {
            claimedTouches->addObject(touch);
        }
    }
}

void ScrollLayerExt::cancelAndStoleTouch(cocos2d::CCTouch *touch, cocos2d::CCEvent *event)
{
    CCSet* set = new CCSet();
    set->addObject(touch);
    set->autorelease();
    m_cancellingTouches = true;
    auto touchDispather = CCDirector::sharedDirector()->getTouchDispatcher();
    touchDispather->touchesCancelled(set, event);
    // Unknown Call.... (note from me, firee: i found your call! ^^^)
    // (**(code **)(*piVar1 + 0x34))(piVar1,this_00,event);
    // *(undefined *)&this[1].vtable = 0; // this is supposed to be m_cancellingTouches
    m_cancellingTouches = false;
    claimTouch(touch);


    /*
  piVar1 = (int *)cocos2d::CCDirector::sharedDirector();
  piVar1 = (int *)(**(code **)(*piVar1 + 0x50))();
  (**(code **)(*piVar1 + 0x34))(piVar1,this_00,param_2);
  this->m_cancellingTouches = false;
  claimTouch(this,param_1);
    */
}


ScrollLayerExt::ScrollLayerExt(CCRect const& rect, bool scrollWheelEnabled, bool vertical) :
    CCScrollLayerExt(rect) {
    m_scrollWheelEnabled = scrollWheelEnabled;

    m_disableVertical = !vertical;
    m_disableHorizontal = vertical;
    m_cutContent = true;

    m_contentLayer->removeFromParent();
    m_contentLayer = GenericContentLayer::create(rect.size.width, rect.size.height);
    m_contentLayer->setID("content-layer");
    m_contentLayer->setAnchorPoint({ 0, 0 });
    this->addChild(m_contentLayer);

    m_touchStartPosition2 = cocos2d::CCPointMake(0.f, 0.f);
	m_touchPosition2 = cocos2d::CCPointMake(0.f, 0.f);

    this->setID("ScrollLayerExt"_spr);

    this->setMouseEnabled(true);
    this->setTouchEnabled(true);
}

void ScrollLayerExt::visit() {
    if (m_cutContent && this->isVisible()) {
        glEnable(GL_SCISSOR_TEST);
            
        if (this->getParent()) {
            // CCPoint const offset = this->isIgnoreAnchorPointForPosition() 
            //     ? ccp(0, 0) : CCPoint(this->getContentSize() * -this->getAnchorPoint());

            auto const bottomLeft = this->convertToWorldSpace(ccp(0, 0));
            auto const topRight = this->convertToWorldSpace(this->getContentSize());
            CCSize const size = topRight - bottomLeft;

            CCEGLView::get()->setScissorInPoints(bottomLeft.x, bottomLeft.y, size.width, size.height);
        }
    }

    CCNode::visit();

    if (m_cutContent && this->isVisible()) {
        glDisable(GL_SCISSOR_TEST);
    }
}

void ScrollLayerExt::scrollToTop() {
    auto listTopScrollPos = -m_contentLayer->getContentHeight() + this->getContentHeight();
    //m_contentLayer->setPositionY(listTopScrollPos);
}

ScrollLayerExt* ScrollLayerExt::create(CCRect const& rect, bool scroll, bool vertical) {
    auto ret = new ScrollLayerExt(rect, scroll, vertical);
    ret->autorelease();
    return ret;
}

void ScrollLayerExt::setCallbackMove(utils::MiniFunction<void()> callbackMove) {
    m_callbackMove = callbackMove;
}
void ScrollLayerExt::setCallbackEnd(utils::MiniFunction<void()> callbackEnd) {
    m_callbackEnd = callbackEnd;
}

ScrollLayerExt* ScrollLayerExt::create(CCSize const& size, bool scroll, bool vertical) {
    return ScrollLayerExt::create({ 0, 0, size.width, size.height }, scroll, vertical);
}

void ScrollLayerExt::fixTouchPrio() {
    auto oldThis = this;
    if (auto delegate = typeinfo_cast<CCTouchDelegate*>(this)) {
        if (auto handler = CCTouchDispatcher::get()->findHandler(delegate)) {
            Loader::get()->queueInMainThread([this, handler, delegate, oldThis]() {
                if (oldThis != nullptr) {
                    if (auto dispatcher = CCTouchDispatcher::get()) {
                        dispatcher->setPriority(handler->m_nPriority - 2, delegate);
                    }
                }
            });
        }
    }
}

void ScrollLayerExt::touchFinish(CCTouch* touch) {
    if (m_callbackEnd) m_callbackEnd();
    auto touchPoint = cocos2d::CCDirector::sharedDirector()->convertToGL(touch->getLocationInView());
    if (touch == this->m_touchStart) {
        //auto pvVar1 = (CCMenuItemSpriteExtra *)itemForTouch((CCTouch *)this);
        auto pvVar1 = nullptr;
        if (pvVar1 != nullptr && !m_notAtEndOfScroll) {
        }
    }
    m_touchStartPosition2 = m_touchPosition2;
    m_touchMoved = false;
}
