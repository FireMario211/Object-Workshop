#include "EditPopup.hpp"
#include "../../utils.hpp"
#include "../../config.hpp"
#include "../../nodes/ScrollLayerExt.hpp"
#include "FiltersPopup.hpp"
#include "Geode/utils/cocos.hpp"
#include "../../nodes/ItemNode.hpp"

bool EditPopup::init(ObjectData obj, std::unordered_set<std::string> availableTags, UserData user) {
    if (!Popup::init(350.f, 280.f)) return false;
    m_availableTags = availableTags;
    m_object = obj;
    m_user = user;
    this->setTitle(fmt::format("Edit {}", obj.name));
    m_objName = TextInput::create(300.0F, "Object Name", "bigFont.fnt");
    m_objName->setScale(0.8);
    m_objName->setMaxCharCount(64);
    m_objName->setCommonFilter(CommonFilter::Any);
    m_objName->setString(obj.name);
    m_mainLayer->addChildAtPosition(m_objName, Anchor::Center, {0, -20});
    m_oldObjectString = m_object.objectString;
    
    m_objDesc = TextInputNode::create("Description [Optional]", 300, {270.F, 60.F}, 90);
    m_mainLayer->addChildAtPosition(m_objDesc, Anchor::Center, {0, -65});
    m_objDesc->addChildAtPosition(m_objDesc->getInput(), Anchor::Center);
    obj.description = Utils::replaceAll(obj.description, "\r\n", "\\n");
    obj.description = Utils::replaceAll(obj.description, "\n", "\\n");
    updateDescObj(obj.description);
    m_objDesc->setString(obj.description);
    //m_objDesc->getInput()->m_textArea->setScale(Utils::calculateScale(obj.description, 50, 300, 1.0F, 0.35F));
    m_objDesc->setUpdateCallback([this](std::string text) {
        updateDescObj(text);
    });
    Build<ButtonSprite>::create(
        CCSprite::createWithSpriteFrameName("GJ_filterIcon_001.png"),
        30,
        0,
        .0F,
        1.0F,
        false,
        "GJ_button_04.png",
        false
    ).scale(0.75f).intoMenuItem([this](){
        FiltersPopup::create(m_availableTags, m_object.tags, true, 0, true, [this](std::unordered_set<std::string> selectedTags, bool, bool, bool) {
            m_object.tags = selectedTags;
        })->show();
    }).parentAtPos(m_buttonMenu, Anchor::Bottom, {-50, 28});
    Build<ButtonSprite>::create("Update", "bigFont.fnt", "GJ_button_01.png").scale(0.8f).intoMenuItem(this, menu_selector(EditPopup::onUpdateBtn)).parentAtPos(m_buttonMenu, Anchor::Bottom, {30, 28});

    auto toggleOffSpr = CCSprite::createWithSpriteFrameName("GJ_checkOff_001.png");
    auto toggleOnSpr = CCSprite::createWithSpriteFrameName("GJ_checkOn_001.png");
    toggleOffSpr->setScale(0.75F);
    toggleOnSpr->setScale(0.75F);
    auto toggleBtn = CCMenuItemToggler::create(
        toggleOffSpr,
        toggleOnSpr,
        this,
        menu_selector(EditPopup::onOverwriteBtn)
    );
    m_overwriteInfo = MDTextArea::create("Enabling <cg>Overwrite</c> will allow you to <cy>update the current object</c>, rather than needing to <cl>reupload the object</c>.\n\nPlease note that if you are not <cg>Verified</c>, and you overwrite the object, the object will <cl>go back to pending</c>, meaning you will have to wait until a <cp>Reviewer</c> can <cg>accept</c> the object.\n\nPlease ensure that if you do overwrite the object, the object does not violate any <cr>upload rules</c>.", {280.F, 80.F});
    m_buttonMenu->addChildAtPosition(m_overwriteInfo, Anchor::Center, {0,65});

    Build<CCLabelBMFont>::create("Overwrite", "bigFont.fnt").scale(.75f).parentAtPos(m_buttonMenu, Anchor::Center, {20, 8});
    m_buttonMenu->addChildAtPosition(toggleBtn, Anchor::Center, {-65,8});

    m_previewBG = CCScale9Sprite::create("square02_small.png");
    m_previewBG->setOpacity(60);
    m_previewBG->setContentSize({ 295.F - 20.F, 82.F });
    auto previewLabel = CCLabelBMFont::create("Select an Object (For overwriting)", "goldFont.fnt");
    previewLabel->setScale(0.425F);
    m_previewBG->addChildAtPosition(previewLabel, Anchor::Top, {0,-8});

    if (auto editor = EditorUI::get()) {
        if (obj.authorAccId == user.account_id) {
            auto scrollLayer = ScrollLayerExt::create({ 0, 0, 275.F, 60.f }, true);
            scrollLayer->setTouchEnabled(true);

            auto content = Build<CCMenu>::create().zOrder(2).layout(
                RowLayout::create()
                    ->setAxisAlignment(AxisAlignment::Start)
                    ->setAutoScale(false)
                    ->setCrossAxisOverflow(true)
                    ->setGap(8)
                    ->setGrowCrossAxis(true)
            ).pos(0,0).anchorPoint(0,0).collect();

            auto customObjKeys = CCArrayExt<CCString*>(GameManager::get()->getOrderedCustomObjectKeys());
            if (customObjKeys.empty()) {
                Build<CCLabelBMFont>::create("You have no objects.", "chatFont.fnt").scale(0.75f).parentAtPos(m_mainLayer, Anchor::Center);
            } else {
                for (auto& key : customObjKeys) {
                    auto obj = GameManager::get()->stringForCustomObject(key->intValue());
                    if (obj.empty()) continue;
                    auto cell = CCMenuItemExt::createSpriteExtra(ItemNode::create(editor->m_editorLayer, obj), [this, obj](auto node) {
                        int tag = node->getTag();
                        for (auto& btn : CCArrayExt<CCMenuItemSpriteExtra*>(node->getParent()->getChildren())) {
                            if (btn->getTag() == tag) {
                                m_object.objectString = std::string(obj);
                                static_cast<ItemNode*>(btn->getChildren()->objectAtIndex(0))->toggle(true);
                            } else {
                                static_cast<ItemNode*>(btn->getChildren()->objectAtIndex(0))->toggle(false);
                            }
                        }
                    });
                    cell->setTag(key->intValue());
                    content->addChild(cell);
                }
                scrollLayer->m_contentLayer->setContentSize({
                    275.f,
                    60.f
                });
                content->setContentSize(scrollLayer->m_contentLayer->getContentSize());
                scrollLayer->m_contentLayer->addChild(content);
                if (customObjKeys.size() <= 7) {
                    scrollLayer->setTouchEnabled(false);
                    scrollLayer->setMouseEnabled(false);
                }
            }
            content->updateLayout();
            scrollLayer->m_contentLayer->setContentHeight(content->getContentHeight());
            scrollLayer->moveToTop();
            m_previewBG->addChild(scrollLayer);
        }
        m_mainLayer->addChildAtPosition(m_previewBG, Anchor::Center, {0, 65});
    }
    m_previewBG->setVisible(false);

    return true;
}

void EditPopup::updateDescObj(std::string text) {
    m_objDesc->getInput()->m_textArea->m_width = 300.0F / Utils::calculateScale(text, 50, 300, 1.0F, 0.5F);
    m_objDesc->getInput()->setScale(Utils::calculateScale(text, 50, 300, 0.75F, 0.45F));
    m_objDesc->getInput()->setPosition({
        Utils::calculateScale(text, 50, 300, 100, 60),
        Utils::calculateScale(text, 50, 300, 25, 20)
    });
}

void EditPopup::onOverwriteBtn(CCObject*) {
    if (m_object.authorAccId != m_user.account_id) return FLAlertLayer::create("Error", "You cannot <cr>overwrite</c> an object that is not your own!", "OK")->show();
    m_previewBG->setVisible(!m_previewBG->isVisible());
    m_overwriteInfo->setVisible(!m_overwriteInfo->isVisible());
}

void EditPopup::onUpdateBtn(CCObject*) {
    auto token = Mod::get()->getSettingValue<std::string>("token");
    m_listener.cancel();
    auto notif = Notification::create("Updating Object...", NotificationIcon::Loading);
    notif->show();
    if (m_objName != nullptr && !m_objDesc->getString().empty()) {
        m_object.name = m_objName->getString();
    }
    m_object.description = "[No description provided]";
    if (m_objDesc != nullptr && !m_objDesc->getString().empty()) {
        m_object.description = Utils::replaceAll(m_objDesc->getString(), "\\n", "\n");
    }
    web::WebRequest req = web::WebRequest();
    req.userAgent(USER_AGENT);
    auto certValid = Mod::get()->getSettingValue<bool>("cert-valid");
    if (!certValid) {
        req.certVerification(certValid);
    }
    auto myjson = matjson::Value();
    myjson.set("token", token);
    myjson.set("name", m_object.name);
    myjson.set("description", m_object.description);
    myjson.set("tags", m_object.tags);
    req.header("Content-Type", "application/json");
    req.bodyJSON(myjson);

    m_listener.spawn(
        req.post(fmt::format("{}/objects/{}/update", HOST_URL, m_object.id)),
        [this, notif, token](web::WebResponse value) {
            auto jsonRes = value.json().unwrapOrDefault();
            if (Utils::notifError(jsonRes)) return;
            notif->hide();
            auto message = jsonRes.get("message");
            if (message.isOk()) {
                if (m_previewBG->isVisible()) { // assume they want to overwrite
                    if (m_oldObjectString != m_object.objectString) {
                        web::WebRequest req = web::WebRequest();
                        req.userAgent(USER_AGENT);
                        auto certValid = Mod::get()->getSettingValue<bool>("cert-valid");
                        if (!certValid) {
                            req.certVerification(certValid);
                        }
                        auto myjson = matjson::Value();
                        myjson.set("token", token);
                        myjson.set("data", m_object.objectString);
                        req.header("Content-Type", "application/json");
                        req.bodyJSON(myjson);
                        async::spawn(
                            req.post(fmt::format("{}/objects/{}/overwrite", HOST_URL, m_object.id)),
                            [this](web::WebResponse value) {
                                auto jsonRes = value.json().unwrapOrDefault();
                                if (Utils::notifError(jsonRes)) return;
                                log::info("Overwrote object.");
                                this->onClose(nullptr);
                                Notification::create("Updated object!", NotificationIcon::Success)->show();
                            }
                        );
                        return this->setVisible(false);
                    }
                }
                this->onClose(nullptr);
                Notification::create(message.unwrap().asString().unwrapOrDefault(), NotificationIcon::Success)->show();
            } else {
                log::error("Unknown response, expected message. {}", message.err());
                Notification::create("Got an unknown response, check logs for details.", NotificationIcon::Warning)->show();
                this->onClose(nullptr);
            }
        }
    );
}
