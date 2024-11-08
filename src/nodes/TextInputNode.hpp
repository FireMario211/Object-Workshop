#pragma once
#include <Geode/Geode.hpp>

using namespace geode::prelude;

class TextInputNode : public CCNode, TextInputDelegate {
    protected:
        CCSize m_bgSize;
        CCScale9Sprite* m_bg;
        CCTextInputNode* m_input;
        std::string m_text;
        void updateDescText(std::string string);
        std::function<void()> m_textInputChanged = [](){};
        std::function<void()> m_textInputClosed = [](){};
        std::function<void(std::string)> m_updateText = [](std::string){};
    
        void textInputOpened(CCTextInputNode*) override {}
        void textChanged(CCTextInputNode*) override;
        void textInputClosed(CCTextInputNode*) override;
        virtual bool init(std::string placeholder, unsigned int maxLength, CCSize bgSize, float bgOpacity, float scale);
    public:
        void setInputWidth(float width) {
            m_input->setContentWidth(width);
            m_input->m_textArea->m_width = width + (width / 2);
        }
        void setString(std::string text) {
            m_input->setString(text);
            m_text = text;
        }
        bool cancel = false;
        std::string getString() { return m_text; };
        CCTextInputNode* getInput() { return m_input; };
        CCScale9Sprite* getBackground() { return m_bg; };
        CCSize getSize() { return m_bgSize; };
        void setInputChangedCallback(std::function<void()> callback) {
            m_textInputChanged = callback;
        }
        void setInputClosedCallback(std::function<void()> callback) {
            m_textInputClosed = callback;
        }
        void setUpdateCallback(std::function<void(std::string)> callback) {
            m_updateText = callback;
        }
        static TextInputNode* create(std::string placeholder, unsigned int maxLength, CCSize bgSize, float bgOpacity, float scale) {
            auto ret = new TextInputNode();
            if (ret->init(placeholder, maxLength, bgSize, bgOpacity, scale)) {
                ret->autorelease();
                return ret;
            }
            delete ret;
            return nullptr;
        }
        static TextInputNode* create(std::string placeholder, unsigned int maxLength, CCSize bgSize, float bgOpacity) {
            return TextInputNode::create(placeholder, maxLength, bgSize, bgOpacity, 1.0F);
        }
        static TextInputNode* create(std::string placeholder, unsigned int maxLength, CCSize bgSize) {
            return TextInputNode::create(placeholder, maxLength, bgSize, 100.F);
        }
        static TextInputNode* create(std::string placeholder, unsigned int maxLength) {
            return TextInputNode::create(placeholder, maxLength, {360, 60});
        }
        static TextInputNode* create(std::string placeholder) {
            return TextInputNode::create(placeholder, 100);
        }
};
