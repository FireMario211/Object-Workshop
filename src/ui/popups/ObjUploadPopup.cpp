#include "ObjUploadPopup.hpp"
#include "../../config.hpp"
#include "../../utils.hpp"
#include "FiltersPopup.hpp"
#include "Geode/ui/Popup.hpp"
#include "../auth/AuthLoadLayer.hpp"

extern std::unordered_set<std::string> g_availableTags;

namespace {
    class ItemNode : public CCNode {
        protected:
            CCSprite* m_bgSprBehind;
            bool init(LevelEditorLayer* editorLayer, gd::string data) {
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
    class RulesPopup : public geode::Popup {
        protected:
            CCMenuItemSpriteExtra* m_okBtn;
            void toggleOk(bool state) {
                m_okBtn->setEnabled(state);
                static_cast<ButtonSprite*>(m_okBtn->getChildren()->objectAtIndex(0))->setOpacity(state ? 255 : 170);
            }
            bool init(std::function<void()> callback) {
                if (!Popup::init({270,260})) return false;
                this->setTitle("Rules");
                m_mainLayer->addChildAtPosition(MDTextArea::create("1. Do not <cy>upload spam, duplicate, or useless objects.</c>\n2. Do not upload <cy>any stolen art</c>, objects that <cy>violate copyright</c>, or objects <cy>without the creators permission.</c>\n3. Do not upload objects that relate to anything that is <cy>inappropriate, explicit, sexual, or violent.</c>\n4. Use an <cy>appropriate name, and tags</c> when uploading.\n\nBreaking these rules will result in a <cr>temporary ban</c>, and possibly a <cr>permanent one</c> if <cy>too severe or repeated.</c>\n\nBy checking the box on **I agree**, you agree to these rules.", {240, 160}), Anchor::Center, {0,20});
                Build<CCLabelBMFont>::create("I agree.", "bigFont.fnt").scale(0.6f).parentAtPos(m_mainLayer, Anchor::Bottom, {15, 55});
                auto okBtn = Build<ButtonSprite>::create("OK").intoMenuItem([this, callback = std::move(callback)]() {
                    this->onClose(nullptr);
                    callback();
                }).parentAtPos(m_buttonMenu, Anchor::Bottom, {0, 25}).store(m_okBtn);
                static_cast<ButtonSprite*>(m_okBtn->getChildren()->objectAtIndex(0))->setCascadeOpacityEnabled(true);
                Build<CCMenuItemToggler>::createToggle(Build<CCSprite>::createSpriteName("GJ_checkOff_001.png").scale(0.65f).collect(), Build<CCSprite>::createSpriteName("GJ_checkOn_001.png").scale(0.65f).collect(), [this](CCMenuItemToggler* toggler){
                    toggleOk(!toggler->isToggled());
                }).parentAtPos(m_buttonMenu, Anchor::Bottom, {-45, 55});
                toggleOk(false);
                return true;
            }
        public:
            static RulesPopup* create(std::function<void()> callback) {
                auto ret = new RulesPopup();
                if (ret->init(callback)) {
                    ret->autorelease();
                    return ret;
                }
                delete ret;
                return nullptr;
            }
    };
}


bool ObjUploadPopup::init(UserData user) {
    if (!Popup::init(300.f, 275.f)) return false;
    m_token = Mod::get()->getSettingValue<std::string>("token");
    Build<CCSprite>::createSpriteName("GJ_arrow_01_001.png").intoMenuItem(this, menu_selector(ObjUploadPopup::onSelectPickPage)).parentAtPos(m_buttonMenu, Anchor::Left, {-30, 0}).store(m_prevBtn);
    Build<CCSprite>::createSpriteName("GJ_arrow_01_001.png").flipX(true).intoMenuItem(this, menu_selector(ObjUploadPopup::onObjectDetailsPage)).enabled(false).opacity(100).parentAtPos(m_buttonMenu, Anchor::Right, {30, 0}).store(m_nextBtn);

    // Select
    m_scrollLayer = ScrollLayerExt::create({ 0, 0, 272.F, 230.f }, true);
    m_scrollLayer->setTouchEnabled(true);
    Build<CCMenu>::create().zOrder(2).layout(
        RowLayout::create()
            ->setAxisAlignment(AxisAlignment::Start)
            ->setAutoScale(false)
            ->setCrossAxisOverflow(true)
            ->setGap(8)
            ->setGrowCrossAxis(true)
    ).pos(0,0).anchorPoint(0,0).registerTouchDispatcher().store(m_content);
    m_scrollLayer->registerWithTouchDispatcher();
    if (auto editor = EditorUI::get()) {
        m_previewBG = ExtPreviewBG::create(editor->m_editorLayer, "", {235.F, 100.F});
        m_previewBG->setTouchEnabled(false);
        m_previewBG->setMouseEnabled(false);
        m_mainLayer->addChildAtPosition(m_previewBG, Anchor::Center, {-16.5f, 50});

        Build<CCScale9Sprite>::create("square02_small.png").opacity(50).scale(0.8f).contentSize(27,124).with([this](auto node) {
            m_slider = Slider::create(this, menu_selector(ObjUploadPopup::onSliderZoom), 1.0f);
            m_slider->setValue((m_previewBG->getScale() / (float)MAX_ZOOM));
            m_oldSliderValue = m_slider->getValue();
            m_slider->setBarVisibility(false);
            m_slider->setRotated(true);
            m_slider->hideGroove(true);
            m_slider->setMaxOffset(75.F);
            m_slider->setContentSize({0, 0});
            node->addChildAtPosition(m_slider, Anchor::Center);
        }).store(m_sliderBg).parentAtPos(m_mainLayer, Anchor::Right, {-27, 50});

        auto customObjKeys = CCArrayExt<CCString*>(GameManager::get()->getOrderedCustomObjectKeys());
        if (customObjKeys.empty()) {
            m_nextBtn->setVisible(false);
            Build<CCLabelBMFont>::create("You have no objects.", "chatFont.fnt").scale(0.75f).parentAtPos(m_mainLayer, Anchor::Left, {75, 0});
        } else {
            for (auto& key : customObjKeys) {
                auto obj = GameManager::get()->stringForCustomObject(key->intValue());
                if (obj.empty()) continue;
                auto ccObj = CCString::create(obj);
                auto cell = CCMenuItemSpriteExtra::create(ItemNode::create(editor->m_editorLayer, obj), this, menu_selector(ObjUploadPopup::onSelectObject));
                cell->setTag(key->intValue());
                cell->setUserObject(ccObj);
                m_content->addChild(cell);
            }
            m_scrollLayer->m_contentLayer->setContentSize({
                272.f,
                230.f
            });
            m_content->setContentSize(m_scrollLayer->m_contentLayer->getContentSize());
            m_scrollLayer->m_contentLayer->addChild(m_content);
            if (customObjKeys.size() <= 42) {
                m_scrollLayer->setTouchEnabled(false);
                m_scrollLayer->setMouseEnabled(false);
            }
        }
    }
    m_content->updateLayout();
    m_scrollLayer->m_contentLayer->setContentHeight(m_content->getContentHeight());
    Build<CCScale9Sprite>::create("square02_small.png").opacity(60).contentSize(m_scrollLayer->getContentSize()).store(m_bg).parentAtPos(m_mainLayer, Anchor::Center, {0, -12}).collect()->addChild(m_scrollLayer);

    m_scrollLayer->moveToTop();
    Loader::get()->queueInMainThread([this]() {
        m_scrollLayer->fixTouchPrio();
        Utils::forceFixPrio(m_content);
    });
    // Details

    m_objName = TextInput::create(335.0F, "Object Name", "bigFont.fnt");
    m_objName->setScale(0.8);
    m_objName->setMaxCharCount(64);
    m_objName->setCommonFilter(CommonFilter::Any);
    m_mainLayer->addChildAtPosition(m_objName, Anchor::Center, {0, -20});

#ifndef GEODE_IS_ANDROID32
    auto textArea = TextArea::create("", "chatFont.fnt", 1.0F, 335.0F, {0.5, 0.5}, 20.0F, true);
    //             TextArea::create(&local_64,"chatFont.fnt",,0x439d8000,this_03,0x41a00000,1);
#endif
    m_objDesc = TextInput::create(335.0F, "Description [Optional] (Use \\n for new line)", "chatFont.fnt");
#ifndef GEODE_IS_ANDROID32
    m_objDesc->getInputNode()->addTextArea(textArea);
    m_objDesc->getInputNode()->m_cursor->setOpacity(0);
#endif
    m_objDesc->getBGSprite()->setContentSize({535.0F, 100.0F});
    m_objDesc->setMaxCharCount(300);
    m_objDesc->setCommonFilter(CommonFilter::Any);
    m_mainLayer->addChildAtPosition(m_objDesc, Anchor::Center, {0, -65});
#ifndef GEODE_IS_ANDROID32
    m_objDesc->setCallback(
        [this, textArea](std::string p0) {
            m_objDesc->getInputNode()->m_textLabel->setOpacity((p0.empty()) ? 255 : 0);
            textArea->setScale(Utils::calculateScale(p0, 50, 300, 0.9F, 0.35F));
            textArea->m_width = 220.0F / Utils::calculateScale(p0, 50, 300, 1.0F, 0.32F);
            textArea->setString(m_objDesc->getInputNode()->getString());
            //textArea->setString(p0.data());
        }
    );
#endif
    Build<ButtonSprite>::create("Upload", 205, true, "bigFont.fnt", "GJ_button_01.png", 30.f, 0.8f).intoMenuItem([this]() {
        if (Mod::get()->getSavedValue<int>("rule_ver") != RULES_VERSION) {
            RulesPopup::create([this]() {
                Mod::get()->setSavedValue("rule_ver", RULES_VERSION);
                showUpload();
            })->show();
        } else {
            showUpload();
        }
    }).store(m_uploadBtn).parentAtPos(m_buttonMenu, Anchor::Bottom, {25, 25});
    Build<ButtonSprite>::create(
        CCSprite::createWithSpriteFrameName("GJ_filterIcon_001.png"),
        30,
        0,
        .0F,
        1.0F,
        false,
        "GJ_button_04.png",
        false
    ).scale(0.75f).intoMenuItem([this]() {
        FiltersPopup::create(g_availableTags, m_filterTags, true, 0, true, [this](std::unordered_set<std::string> selectedTags, bool, bool, bool) {
            m_filterTags = selectedTags;
        })->show();
    }).store(m_filterBtn).parentAtPos(m_buttonMenu, Anchor::BottomLeft, {31, 25});
    onSelectPickPage(nullptr);
    return true;
}

void ObjUploadPopup::showUpload() {
    //auto popup = UploadActionPopup::create(this, "Uploading...");
    //popup->show();
    if (m_previewBG->getData().empty() || m_objName == nullptr || m_objDesc == nullptr) return FLAlertLayer::create("Error", "something must have gone very wrong\nfor this to happen.", "OK")->show();
    if (m_filterTags.size() == 0) return FLAlertLayer::create("Error", "You must <cy>set a tag</c>!\nClick on the grey filter button!", "OK")->show();
    if (m_filterTags.size() > 5) return FLAlertLayer::create("Error", "You cannot set more than <cy>5 tags</c>!", "OK")->show();
    if (m_objName != nullptr && m_objName->getString().empty()) return FLAlertLayer::create("Error", "You must enter an\n<cy>object name</c>!", "OK")->show();
    auto loadLayer = AuthLoadLayer::create();
    loadLayer->show();
    ObjectData obj = {
        0,
        m_objName->getString(),
        "[No description provided]"
    };
    obj.objectString = m_previewBG->getData();
    if (obj.objectString == "") return FLAlertLayer::create("Error", "You must <cy>select an object</c>!", "OK")->show();
    if (m_objDesc != nullptr && !m_objDesc->getString().empty()) {
        obj.description = Utils::replaceAll(m_objDesc->getString(), "\\n", "\n");
    }
    obj.tags = m_filterTags;
    m_listener.cancel();
    m_buttonMenu->setEnabled(false);
    m_filterTags.clear();
    web::WebRequest req = web::WebRequest();
    req.userAgent(USER_AGENT);
    auto certValid = Mod::get()->getSettingValue<bool>("cert-valid");
    if (!certValid) {
        req.certVerification(certValid);
    }
    auto myjson = matjson::Value();
    myjson.set("token", m_token);
    myjson.set("name", obj.name);
    myjson.set("description", obj.description);
    myjson.set("tags", obj.tags);
    myjson.set("data", obj.objectString);
    req.header("Content-Type", "application/json");
    req.bodyJSON(myjson);
    m_listener.spawn(
        req.post(fmt::format("{}/objects/upload", HOST_URL)),
        [this, loadLayer](web::WebResponse value) {
            loadLayer->finished();
            std::string errContent;
            auto jsonRes = value.json().unwrapOrDefault();
            if (!jsonRes.isObject()) {
                geode::log::error("Response isn't object. Got this instead: {}", jsonRes.dump());
                errContent = "An unknown error occured.";
            }
            auto isError = jsonRes.get("error");
            if (isError.isOk()) {
                errContent = isError.unwrap().asString().unwrapOrDefault();
            } else if (!value.ok()) {
                errContent = "An unknown error occured.";
            }
            if (!errContent.empty()) {
                FLAlertLayer::create("Error", errContent.c_str(), "OK")->show();
                m_buttonMenu->setEnabled(true);
            } else {
                geode::createQuickPopup("Success!", "Your object is now <cy>pending for review</c>! You can view your pending objects by tapping on your profile icon.", "OK", nullptr, [this](FLAlertLayer *, bool) {
                    this->onClose(nullptr);
                });
            }

        }
    );
}

void ObjUploadPopup::onSelectObject(CCObject* ret) {
    auto node = static_cast<CCMenuItemSpriteExtra*>(ret);
    if (m_selectedIndex == node->getTag()) return;
    m_selectedIndex = node->getTag();
    for (auto& btn : CCArrayExt<CCMenuItemSpriteExtra*>(m_content->getChildren())) {
        if (btn->getTag() == m_selectedIndex) {
            CCString* ccData = static_cast<CCString*>(node->getUserObject());
            std::string data = std::string(ccData->getCString());
            m_previewBG->setNewPreview(data);
            m_previewBG->resetZoom();
            static_cast<ItemNode*>(btn->getChildren()->objectAtIndex(0))->toggle(true);
        } else {
            static_cast<ItemNode*>(btn->getChildren()->objectAtIndex(0))->toggle(false);
        }
    }
    m_nextBtn->setEnabled(true);
    m_nextBtn->setOpacity(255);
}

// Page 1
void ObjUploadPopup::onSelectPickPage(CCObject*) {
    m_prevBtn->setEnabled(false);
    m_prevBtn->setOpacity(100);

    m_scrollLayer->setVisible(true);
    m_bg->setVisible(true);

    m_previewBG->setTouchEnabled(false);
    m_previewBG->setMouseEnabled(false);
    m_previewBG->setVisible(false);
    m_sliderBg->setVisible(false);
    m_objName->setVisible(false);
    m_objDesc->setVisible(false);
    m_uploadBtn->setVisible(false);
    m_filterBtn->setVisible(false);

    if (!m_previewBG->getData().empty()) {
        m_nextBtn->setEnabled(true);
        m_nextBtn->setOpacity(255);
    }
    this->setTitle("Select an Object");
}

// Page 2
void ObjUploadPopup::onObjectDetailsPage(CCObject*) {
    m_prevBtn->setEnabled(true);
    m_prevBtn->setOpacity(255);
    m_nextBtn->setEnabled(false);
    m_nextBtn->setOpacity(100);

    m_scrollLayer->setVisible(false);
    m_bg->setVisible(false);

    m_previewBG->setVisible(true);
    m_previewBG->setTouchEnabled(true);
    m_previewBG->setMouseEnabled(true);
    m_sliderBg->setVisible(true);
    m_objName->setVisible(true);
    m_objDesc->setVisible(true);
    m_uploadBtn->setVisible(true);
    m_filterBtn->setVisible(true);

    this->setTitle("Object Details");
}
