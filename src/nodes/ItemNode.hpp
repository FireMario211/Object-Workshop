#pragma once
class ItemNode : public cocos2d::CCNode {
    protected:
        cocos2d::CCSprite* m_bgSprBehind;
        bool init(LevelEditorLayer* editorLayer, gd::string data);
    public:
        void toggle(bool state) {
            m_bgSprBehind->setVisible(state);
        }
        static ItemNode* create(LevelEditorLayer* editorLayer, gd::string data) {
            auto pRet = new ItemNode();
            if (pRet->init(editorLayer, data)) {
                pRet->autorelease();
                return pRet;
            }
            delete pRet;
            return nullptr;
        }
};
