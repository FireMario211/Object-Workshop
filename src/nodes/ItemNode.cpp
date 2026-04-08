#include "ItemNode.hpp"
#include "../utils.hpp"
using namespace geode::prelude;

bool ItemNode::init(LevelEditorLayer* editorLayer, gd::string data) {
    if (!CCNode::init()) return false;
    this->setContentSize({ 32.0f, 32.0f });
    auto bg = Build<CCScale9Sprite>::create("square02_small.png").opacity(60).contentSize(this->getContentSize()).parentAtPos(this, Anchor::Center).collect();
    CCLayerColor* mask = CCLayerColor::create({255, 255, 255});
    mask->setContentSize(bg->getContentSize());
    Build<CCClippingNode>::create().contentSize(this->getContentSize()).anchorPoint(0.5f,0.5f).zOrder(1).with([editorLayer, data, mask](auto node) {
        auto zData = ZStringView(data);
        unsigned int objectCount = std::count(zData.begin(), zData.end(), ';');
        if (!zData.empty()) {
            auto smartBlock = CCArray::create();
            int renderLimit = Mod::get()->getSettingValue<int64_t>("render-objects");
            int preRender = Mod::get()->getSettingValue<int64_t>("prerender-objects");
            CCSprite* sprite = editorLayer->m_editorUI->spriteFromObjectString(data, false, false, renderLimit, smartBlock, (CCArray *)0x0,(GameObject *)0x0);
            editorLayer->updateObjectColors(smartBlock);
            sprite->setScale(((node->getContentSize().height - 6) / sprite->getContentSize().height));
            if (objectCount >= preRender) {
                CCSize contentSize = node->getContentSize();
                sprite->setPosition(contentSize / 2);
                CCRenderTexture* tex = CCRenderTexture::create(contentSize.width, contentSize.height);
                tex->beginWithClear(0, 0, 0, 0);
                sprite->visit();
                tex->end();
                node->addChildAtPosition(tex, Anchor::Center);
            } else {
                node->addChildAtPosition(sprite, Anchor::Center);
            }
            node->setStencil(mask);
        }
    }).parentAtPos(bg, Anchor::Center);
    Build<CCSprite>::create("select_outline.png"_spr).scale(0.4f).visible(false).store(m_bgSprBehind).parentAtPos(this, Anchor::Center);
    return true;
}
