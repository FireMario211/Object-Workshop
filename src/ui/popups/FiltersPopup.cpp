#include "FiltersPopup.hpp"
#include "../../utils.hpp"

bool FiltersPopup::setup(std::unordered_set<std::string> tags, std::unordered_set<std::string> selectedTags, bool uploading, std::function<void(std::unordered_set<std::string>)> callback) {
    m_callback = callback;
    m_selectedTags = selectedTags;

    this->setTitle((uploading) ? "Set Tags" : "Search Filters");

    // loader/src/ui/mods/popups/FiltersPopup.cpp
    auto tagsContainer = CCNode::create();
    tagsContainer->setContentSize(ccp(310, 80));
    tagsContainer->setAnchorPoint({ .5f, .5f });

    auto tagsBG = CCScale9Sprite::create("square02b_001.png");
    tagsBG->setColor({ 0, 0, 0 });
    tagsBG->setOpacity(75);
    tagsBG->setScale(.3f);
    tagsBG->setContentSize(tagsContainer->getContentSize() / tagsBG->getScale());
    tagsContainer->addChildAtPosition(tagsBG, Anchor::Center);

    m_tagsMenu = CCMenu::create();
    m_tagsMenu->setContentSize(tagsContainer->getContentSize() - ccp(10, 10));
    m_tagsMenu->setLayout(
        RowLayout::create()
            ->setDefaultScaleLimits(.1f, 1.f)
            ->setGrowCrossAxis(true)
            ->setCrossAxisOverflow(false)
            ->setAxisAlignment(AxisAlignment::Center)
            ->setCrossAxisAlignment(AxisAlignment::Center)
    );
    tagsContainer->addChildAtPosition(m_tagsMenu, Anchor::Center);

    auto tagsTitleMenu = CCMenu::create();
    tagsTitleMenu->setAnchorPoint({ .5f, 0 });
    tagsTitleMenu->setContentWidth(tagsContainer->getContentWidth());

    auto tagsTitle = CCLabelBMFont::create("Tags", "bigFont.fnt");
    tagsTitleMenu->addChild(tagsTitle);

    tagsTitleMenu->addChild(SpacerNode::create());

    auto resetSpr = CCSprite::createWithSpriteFrameName("GJ_trashBtn_001.png");
    auto resetBtn = CCMenuItemSpriteExtra::create(
        resetSpr, this, menu_selector(FiltersPopup::onResetBtn)
    );
    tagsTitleMenu->addChild(resetBtn);

    tagsTitleMenu->setLayout(
        RowLayout::create()
            ->setDefaultScaleLimits(.1f, .4f)
    );
    tagsContainer->addChildAtPosition(tagsTitleMenu, Anchor::Top, ccp(0, 4));

    m_mainLayer->addChildAtPosition(tagsContainer, Anchor::Top, ccp(0, -85));

    auto okSpr = ButtonSprite::create("OK", "bigFont.fnt", "GJ_button_01.png");
    okSpr->setScale(.7f);
    auto okBtn = CCMenuItemSpriteExtra::create(
        okSpr, this, menu_selector(FiltersPopup::onClose)
    );
    m_buttonMenu->addChildAtPosition(okBtn, Anchor::Bottom, ccp(0, 20));

    m_tagsMenu->removeAllChildren();
    for (auto& tag : tags) {
        auto offSpr = ButtonSprite::create(tag.c_str(), "bigFont.fnt", "geode.loader/white-square.png", .8f);
        offSpr->m_BGSprite->setColor(Utils::generateColorFromString(tag));
        offSpr->m_BGSprite->setOpacity(105);
        offSpr->m_label->setOpacity(105);
        auto onSpr = ButtonSprite::create(tag.c_str(), "bigFont.fnt", "geode.loader/white-square.png", .8f);
        onSpr->m_BGSprite->setColor(Utils::generateColorFromString(tag));
        auto btn = CCMenuItemToggler::create(
            offSpr, onSpr, this, menu_selector(FiltersPopup::onSelectTag)
        );
        btn->m_notClickable = true;
        btn->setUserObject("tag", CCString::create(tag));
        m_tagsMenu->addChild(btn);
    }
    m_tagsMenu->updateLayout();
    this->updateTags();
    return true;
}

CCNode* FiltersPopup::createTags(std::unordered_set<std::string> tags, CCSize contentSize, CCPoint anchorPoint, AxisAlignment alignment) {
    auto node = CCNode::create();
    //node->setContentSize({105, 15});
    node->setContentSize(contentSize);
    node->setAnchorPoint(anchorPoint);
    for (auto& tag : tags) {
        auto spr = ButtonSprite::create(tag.c_str(), "bigFont.fnt", "geode.loader/white-square.png", .8f);
        spr->m_BGSprite->setColor(Utils::generateColorFromString(tag));
        node->addChild(spr);
    }
    node->setLayout(
        RowLayout::create()
            ->setAxisAlignment(alignment)
            ->setAutoScale(true)
            ->setDefaultScaleLimits(0.1F, 0.5F)
            ->setCrossAxisOverflow(false)
            ->setGap(1)
            ->setGrowCrossAxis(true)
            ->setCrossAxisAlignment(AxisAlignment::End)
    );
    node->updateLayout();
    return node;
}

void FiltersPopup::updateTags() {
    for (auto node : CCArrayExt<CCNode*>(m_tagsMenu->getChildren())) {
        if (auto toggle = typeinfo_cast<CCMenuItemToggler*>(node)) {
            auto tag = static_cast<CCString*>(toggle->getUserObject("tag"))->getCString();
            toggle->toggle(m_selectedTags.contains(tag));
        }
    }
}
void FiltersPopup::onSelectTag(CCObject* sender) {
    auto toggle = static_cast<CCMenuItemToggler*>(sender);
    auto tag = static_cast<CCString*>(toggle->getUserObject("tag"))->getCString();
    if (m_selectedTags.contains(tag)) {
        m_selectedTags.erase(tag);
    }
    else {
        m_selectedTags.insert(tag);
    }
    this->updateTags();
}
void FiltersPopup::onResetBtn(CCObject*) {
    m_selectedTags.clear();
    this->updateTags();
}

void FiltersPopup::onClose(CCObject* sender) {
    m_callback(m_selectedTags);
    Popup::onClose(sender);
}

