#pragma once
#include <Geode/Geode.hpp>

using namespace geode::prelude;

class ExtPreviewBG : public CCLayer {
    protected:
        CCSprite* objSprite;
        CCScale9Sprite* m_bg;
        CCPoint m_touchStart;
        CCClippingNode* m_clippingNode;
        float m_oldScale;
        CCPoint m_oldPos;
        float m_currentZoom = 1.0F;
        LevelEditorLayer* m_editorLayer;
        std::string m_data;

        void touchFinished();
        virtual bool init(LevelEditorLayer* editorLayer, std::string objData, CCSize);
        bool ccTouchBegan(CCTouch*, CCEvent*) override;
        void ccTouchMoved(CCTouch*, CCEvent*) override;
    public:
        std::string getData() const {
            return m_data;
        }
        void updateZoom(float amount);
        void setZoom(float amount = 1.0F);
        void resetZoom();
        void setNewPreview(std::string objData);
        static ExtPreviewBG* create(LevelEditorLayer* editorLayer, std::string objData, CCSize contentSize = {124.F, 82.F});
};
