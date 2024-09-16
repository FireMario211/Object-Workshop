#pragma once
#include <Geode/Geode.hpp>

using namespace geode::prelude;

class ExtPreviewBG : public CCLayer {
    protected:
        CCSprite* objSprite;
        CCScale9Sprite* bg;
        CCPoint m_touchStart;
        float m_oldScale;
        float m_currentZoom = 1.0F;

        void touchFinished();
        virtual bool init(std::string objData);
        bool ccTouchBegan(CCTouch*, CCEvent*) override;
        void ccTouchMoved(CCTouch*, CCEvent*) override;
    public:
        void updateZoom(float amount);
        void resetZoom();
        static ExtPreviewBG* create(std::string objData);
};
