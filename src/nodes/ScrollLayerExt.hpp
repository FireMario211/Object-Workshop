#pragma once
#include <Geode/ui/ScrollLayer.hpp>
#include <Geode/Geode.hpp>

using namespace geode::prelude;

// sorta stolen from TableView
/*
class ScrollLayerExt : public CCScrollLayerExt, public CCScrollLayerExtDelegate {
    protected:
        float m_fStartSwipe;
		int m_iState;
		bool m_bStealingTouchInProgress;
		CCTouch* m_pScrollTouch;


        ScrollLayerExt(CCRect const& rect, bool scrollWheelEnabled, bool vertical);
        bool ccTouchBegan(CCTouch*, CCEvent*) override;
        void ccTouchMoved(CCTouch*, CCEvent*) override;
        void ccTouchCancelled(CCTouch*, CCEvent*) override;
        void cancelAndStoleTouch(cocos2d::CCTouch*, cocos2d::CCEvent*);
        void claimTouch(CCTouch* pTouch);
    public:
        static ScrollLayerExt* create(
            cocos2d::CCRect const& rect, bool scrollWheelEnabled = true, bool vertical = true
        );
        static ScrollLayerExt* create(
            cocos2d::CCSize const& size, bool scrollWheelEnabled = true, bool vertical = true
        );
};
*/ 
class ScrollLayerExt : public CCScrollLayerExt, public CCScrollLayerExtDelegate {
    protected:
        bool m_touchOutOfBoundary;
        bool m_scrollWheelEnabled;
        cocos2d::CCTouch* m_touchStart;
        cocos2d::CCPoint m_touchStartPosition2;
        cocos2d::CCPoint m_touchPosition2;
        bool m_touchMoved;
        float m_touchLastY;
        bool m_cancellingTouches;
        std::function<void()> m_callbackMove;
        std::function<void()> m_callbackEnd;

        void cancelAndStoleTouch(cocos2d::CCTouch*, cocos2d::CCEvent*);
        void checkBoundaryOfContent(float);
        void claimTouch(cocos2d::CCTouch*);
        void touchFinish(cocos2d::CCTouch*);

        bool ccTouchBegan(cocos2d::CCTouch*, cocos2d::CCEvent*) override;
        void ccTouchMoved(cocos2d::CCTouch*, cocos2d::CCEvent*) override;

        void ccTouchEnded(cocos2d::CCTouch*, cocos2d::CCEvent*) override;
        void ccTouchCancelled(cocos2d::CCTouch*, cocos2d::CCEvent*) override;
        void scrollWheel(float, float) override;
        void scrollToTop();
        void visit() override;
        CCMenuItemSpriteExtra* itemForTouch(CCTouch*);

        ScrollLayerExt(CCRect const& rect, bool scrollWheelEnabled, bool vertical);
    public:
        void fixTouchPrio();
        void setCallbackMove(std::function<void()> callbackMove);
        void setCallbackEnd(std::function<void()> callbackEnd);
        static ScrollLayerExt* create(
            cocos2d::CCRect const& rect, bool scrollWheelEnabled = true, bool vertical = true
        );
        static ScrollLayerExt* create(
            cocos2d::CCSize const& size, bool scrollWheelEnabled = true, bool vertical = true
        );
};
