// Credits to https://github.com/EclipseMenu/EclipseMenu
#pragma once
#include "ScrollLayerExt.hpp"
#include "../utils.hpp"
//#include <modules/gui/components/combo.hpp>

//static const float s_width = 365.f;
static const float s_width = 965.f;

namespace {
    class ComboComponentNode : public cocos2d::CCMenu {
    protected:
        CCMenuItemSpriteExtra* m_infoButton = nullptr;
        geode::NineSlice* m_background = nullptr;
        CCLabelBMFont* m_valueLabel = nullptr;
        geode::Ref<CCMenuItemSpriteExtra> m_arrowButton = nullptr;
        geode::Ref<ScrollLayerExt> m_scrollLayer;

        static inline ComboComponentNode* s_activeCombo = nullptr;
        std::vector<std::string> m_options;
        Function<void(std::string const&, size_t)> m_callback;
        Function<void(bool)> m_opened;
        size_t m_selectedIndex = -1;
    public:
        static ComboComponentNode* create(float width, std::vector<std::string> options, geode::Function<void(bool)> opened, geode::Function<void(std::string const&, size_t)> callback) {
            auto ret = new ComboComponentNode;
            if (ret->init(width, std::move(options), std::move(opened), std::move(callback))) {
                ret->autorelease();
                return ret;
            }
            delete ret;
            return nullptr;
        }

        ~ComboComponentNode() override {
            if (s_activeCombo == this) {
                s_activeCombo = nullptr;
            }
        }

        void generateSelector() {
            auto const scrollHeight = std::min<float>(m_options.size(), 3.5f) * 80.f;
            m_scrollLayer = ScrollLayerExt::create({s_width * 0.3f, scrollHeight * 0.3f});
            m_scrollLayer->setID("scrollLayer");
            m_scrollLayer->setZOrder(2);
            m_scrollLayer->ignoreAnchorPointForPosition(false);
            m_scrollLayer->m_contentLayer->setLayout(
                geode::ColumnLayout::create()
                    ->setAutoScale(false)
                    ->setAxisReverse(true)
                    ->setAutoGrowAxis(m_scrollLayer->getContentHeight())
                    ->setAxisAlignment(geode::AxisAlignment::End)
                    ->setGap(0),
                false
            );

            auto scrollBlock = cocos2d::CCMenu::create();
            scrollBlock->setScale(0.3f);
            scrollBlock->setContentSize({s_width, scrollHeight});
            scrollBlock->setZOrder(-100);
            scrollBlock->setID("scroll-block"_spr);

            auto scrollBlockButton = CCMenuItemSpriteExtra::create(
                cocos2d::CCSprite::create("GJ_button_01.png"),
                nullptr,
                nullptr
            );
            scrollBlockButton->setContentSize({s_width, scrollHeight});
            scrollBlockButton->setZOrder(1);
            scrollBlockButton->setAnchorPoint({0.5f, 0.5f});
            scrollBlockButton->setOpacity(0);
            scrollBlockButton->m_scaleMultiplier = 1.f;
            scrollBlock->addChildAtPosition(scrollBlockButton, geode::Anchor::Center);
            m_scrollLayer->addChildAtPosition(scrollBlock, geode::Anchor::Center);

            auto scrollBackground = geode::NineSlice::create("geode.loader/black-square.png");//geode::NineSlice::create("square02b_001.png");
            scrollBackground->setID("scrollBackground");
            scrollBackground->setScale(0.3f);
            scrollBackground->setContentSize({s_width, scrollHeight});
            scrollBackground->setColor({15, 15, 15});
            scrollBackground->setZOrder(-2);
            m_scrollLayer->addChildAtPosition(scrollBackground, geode::Anchor::Center);

            for (size_t i = 0; auto const& value : m_options) {
                auto menu = cocos2d::CCMenu::create();
                menu->setContentSize({s_width * 0.3f, 80.f * 0.3f});
                auto itemBG = NineSlice::createWithSpriteFrameName("geode.loader/tab-bg.png");
                itemBG->setColor({0,0,0});
                itemBG->setScale(.5f);
                auto label = CCLabelBMFont::create(value.c_str(), "geode.loader/mdFont.fnt");
                label->setAnchorPoint({0.5f, 0.5f});
                label->limitLabelWidth(menu->getContentSize().width * 1.54f, 0.75f * 1.54f, 0.25f * 1.54f);
                itemBG->setContentSize({570, 40});
                itemBG->addChildAtPosition(label, Anchor::Center);
                if (m_selectedIndex == i) {
                    //label->setColor(tm->getButtonForegroundColor().toCCColor3B());
                } else {
                    //label->setColor(tm->getButtonDisabledForeground().toCCColor3B());
                }
                auto item = CCMenuItemSpriteExtra::create(itemBG, this, menu_selector(ComboComponentNode::selectItem));
                item->setTag(i);
                item->setAnchorPoint({0.5f, 0.5f});
                item->setZOrder(1);
                item->m_scaleMultiplier = 1.1f;
                menu->addChildAtPosition(item, geode::Anchor::Center);

                m_scrollLayer->m_contentLayer->addChild(menu);

                ++i;
            }
            m_scrollLayer->m_contentLayer->updateLayout();
            m_scrollLayer->moveToTop();
        }

        void updateLabel() const {
            if (m_selectedIndex < 0 || m_selectedIndex >= m_options.size()) {
                m_valueLabel->setString("Select an option...");
            } else {
                m_valueLabel->setString(m_options[m_selectedIndex].c_str());
            }
            m_valueLabel->limitLabelWidth(100.f, 1.f, 0.3f);
        }

        void openSelector(CCObject* sender) {
            m_opened(true);
            auto const backgroundBox = m_background->boundingBox();
            auto const globalTop = m_background->convertToWorldSpace(ccp(backgroundBox.getMidX(), backgroundBox.getMaxY()));
            auto const globalBottom = m_background->convertToWorldSpace(ccp(backgroundBox.getMidX(), backgroundBox.getMinY()));
            auto const winSize = cocos2d::CCDirector::get()->getWinSize();

            m_arrowButton->setRotation(270.f);
            m_arrowButton->setTarget(this, menu_selector(ComboComponentNode::closeSelector));

            bool l_createdScroll = false;
            if (!m_scrollLayer) {
                this->generateSelector();
                l_createdScroll = true;
            }
            m_scrollLayer->setVisible(true);

            if (s_activeCombo) {
                s_activeCombo->closeSelector(nullptr);
            }
            s_activeCombo = this;
            if (l_createdScroll) {
                if (globalBottom.y > m_scrollLayer->getContentHeight() + 20.f) {
                    m_scrollLayer->setAnchorPoint({0.5f, 1.f});
                    auto const targetPos = ccp(this->getContentWidth() / 2, this->getContentHeight() / 2.f - backgroundBox.getMidY());
                    if (auto parent = this->getParent(); parent) {
                        auto const parentPos = parent->convertToNodeSpace(this->convertToWorldSpace(targetPos));
                        parent->addChild(m_scrollLayer, 10);
                        m_scrollLayer->setPosition(parentPos);
                    }
                } else {
                    m_scrollLayer->setAnchorPoint({0.5f, 0.f});
                    auto const targetPos = ccp(this->getContentWidth() / 2, this->getContentHeight() / 2.f + backgroundBox.getMidY());
                    if (auto parent = this->getParent(); parent) {
                        auto const parentPos = parent->convertToNodeSpace(this->convertToWorldSpace(targetPos));
                        parent->addChild(m_scrollLayer, 10);
                        m_scrollLayer->setPosition(parentPos);
                    }
                }
            }
        }

        void closeSelector(CCObject* sender) {
            m_opened(false);
            //if (m_scrollLayer) m_scrollLayer->removeFromParent();
            if (m_scrollLayer) m_scrollLayer->setVisible(false);
            m_arrowButton->setRotation(90.f);
            m_arrowButton->setTarget(this, menu_selector(ComboComponentNode::openSelector));
            s_activeCombo = nullptr;
        }

        void selectItem(CCObject* sender) {
            auto index = sender->getTag();
            if (index < 0 || index >= m_options.size()) return;
            m_selectedIndex = index;
            for (size_t i = 0; auto option : m_options) {
                if (i == index) {
                    m_callback(option, index);
                    //label->setColor(ThemeManager::get()->getButtonForegroundColor().toCCColor3B());
                } else {
                    //label->setColor(ThemeManager::get()->getButtonDisabledForeground().toCCColor3B());
                }
                ++i;
            }

            this->updateLabel();
            this->closeSelector(nullptr);
        }

        bool init(float width, std::vector<std::string> options, geode::Function<void(bool)> opened, geode::Function<void(std::string const&, size_t)> callback) {
            if (!CCMenu::init()) return false;
            m_options = std::move(options);
            m_callback = std::move(callback);
            m_opened = std::move(opened);

            this->setContentSize({ width, 28.f });

            //m_background = geode::NineSlice::create("square02b_001.png");
            //"geode.loader/black-square.png"
            m_background = geode::NineSlice::createWithSpriteFrameName("geode.loader/tab-bg.png");
            m_background->setID("background");
            m_background->setAnchorPoint({ 0.5f, 0.5f });
            m_background->setScale(0.3f);
            m_background->setContentSize({s_width, 80.f});
            m_background->setColor({20, 20, 20});
            m_background->setZOrder(-1);
            this->addChildAtPosition(m_background, geode::Anchor::Center);

            auto spr = cocos2d::CCSprite::createWithSpriteFrameName("edit_rightBtn_001.png");
            spr->setScale(0.6f);

            m_arrowButton = CCMenuItemSpriteExtra::create(spr, this, menu_selector(ComboComponentNode::openSelector));
            m_arrowButton->setRotation(90.f);
            if (m_options.empty()) {
                m_arrowButton->setVisible(false);
            }
            this->addChildAtPosition(m_arrowButton, geode::Anchor::Right, { -20.f, 0.f });

            int index = 0;
            m_valueLabel = CCLabelBMFont::create("", "geode.loader/mdFontB.fnt");
            this->addChildAtPosition(m_valueLabel, geode::Anchor::Center, { -10.f, 0.f });
            updateLabel();
            return true;
        }
    };
}
