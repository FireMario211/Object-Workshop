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
    m_input->m_filterSwearWords = false;
    //m_input->setUnicodeText(true);
    //setUnicodeText
    {
        m_input->m_unicodeText = true;
        m_input->m_allowedChars = "";
    }
    //setUnicodeText
    m_input->setAllowedChars(" abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789.,-!?:;)(/\\\"\'`*=+- _%[]<>|@&^#{}%$~");
    m_input->setMaxLabelLength((EmoteSprite::maxEmotesComment() * 17) + maxLength);
    m_input->setLabelPlaceholderColor({200, 200, 200});
    m_input->setDelegate(this);
    // 324.5f for bgSize.width - 65.F?
    m_input->addTextArea(TextArea::create("","chatFont.fnt",scale,bgSize.width - 65.F,{0.5, 0.5}, 20.F * scale,true,1));
    m_input->setParseEmotes(EmoteSprite::maxEmotesComment());
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
