#pragma once
#include <Geode/Geode.hpp>

using namespace geode::prelude;

class ExtPreviewBG : public CCLayer {
    protected:
        CCSprite* objSprite;
        CCScale9Sprite* bg;
        CCPoint m_touchStart;
        CCClippingNode* m_clippingNode;
        float m_oldScale;
        CCPoint m_oldPos;
        float m_currentZoom = 1.0F;

        void touchFinished();
        virtual bool init(std::string objData, CCSize);
        bool ccTouchBegan(CCTouch*, CCEvent*) override;
        void ccTouchMoved(CCTouch*, CCEvent*) override;
    public:
        void updateZoom(float amount);
        void setZoom(float amount = 1.0F);
        void resetZoom();
        static ExtPreviewBG* create(std::string objData, CCSize contentSize = {124.F, 82.F});
};
