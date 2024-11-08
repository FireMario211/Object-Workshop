#include "TextInputNode.hpp"

bool TextInputNode::init(std::string placeholder, unsigned int maxLength, CCSize bgSize, float bgOpacity, float scale) {
    if (!CCNode::init()) return false;
    m_bgSize = bgSize;
    this->setID("TextInputNode"_spr);
    this->setContentSize(bgSize);
    this->setAnchorPoint({0.5, 0.5});
    m_bg = CCScale9Sprite::create("square02b_001.png");
    m_bg->setColor({0,0,0});
    m_bg->setOpacity(bgOpacity);
    m_bg->setContentSize(bgSize);
    // most undocumented thing ever, why does it have to be 0x0 to allow new lines!? i tried chatFont.fnt before but apparently that keeps the CCLabelBMFont!
    m_input = CCTextInputNode::create(bgSize.width, bgSize.height - 10.F, placeholder.c_str(), "Thonburi", 24, 0x0);
    m_input->setAllowedChars(" abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789.,-!?:;)(/\\\"\'`*=+- _%[]<>|@&^#{}%$~");
    m_input->setMaxLabelLength(maxLength);
    m_input->setLabelPlaceholderColor({0xc8, 0xc8, 0xc8});
    m_input->setDelegate(this);
    m_input->addTextArea(TextArea::create("","chatFont.fnt",scale,bgSize.width - 65.F,{0.5, 0.5},20.F * scale,true));
    this->addChildAtPosition(m_bg, Anchor::Center);
    return true;
}

void TextInputNode::textChanged(CCTextInputNode* input) {
    if (cancel) return;
    updateDescText(input->getString());
    m_textInputChanged();
}
void TextInputNode::textInputClosed(CCTextInputNode* input) {
    if (input == nullptr || cancel) return;
    m_text = input->getString();
    updateDescText(m_text);
    m_textInputClosed();
}
void TextInputNode::updateDescText(std::string string) {
    if (cancel) return;
    m_text = string;
    m_updateText(string);
}
