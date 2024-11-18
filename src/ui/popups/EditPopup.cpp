#include "EditPopup.hpp"
#include "../../utils.hpp"
#include "../../config.hpp"
#include "../../nodes/ScrollLayerExt.hpp"
#include "FiltersPopup.hpp"
#include "Geode/utils/cocos.hpp"

bool EditPopup::setup(ObjectData obj, std::unordered_set<std::string> availableTags, UserData user) {
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
    auto filterSpr = ButtonSprite::create(
        CCSprite::createWithSpriteFrameName("GJ_filterIcon_001.png"),
        30,
        0,
        .0F,
        1.0F,
        false,
        "GJ_button_04.png",
        false
    );
    filterSpr->setScale(0.75F);
    auto filterBtn = CCMenuItemExt::createSpriteExtra(filterSpr, [this](CCObject*) {
        FiltersPopup::create(m_availableTags, m_object.tags, true, [this](std::unordered_set<std::string> selectedTags) {
            m_object.tags = selectedTags;
        })->show();
    });
    m_buttonMenu->addChildAtPosition(filterBtn, Anchor::Bottom, {-50, 28});

    auto uploadSpr = ButtonSprite::create("Update", "bigFont.fnt", "GJ_button_01.png");
    uploadSpr->setScale(0.8F);
    auto uploadBtn = CCMenuItemSpriteExtra::create(
        uploadSpr,
        this,
        menu_selector(EditPopup::onUpdateBtn)
    );
    m_buttonMenu->addChildAtPosition(uploadBtn, Anchor::Bottom, {30, 28});

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
    auto overwriteLbl = CCLabelBMFont::create("Overwrite", "bigFont.fnt");
    overwriteLbl->setScale(0.75F);

    m_overwriteInfo = MDTextArea::create("Enabling <cg>Overwrite</c> will allow you to <cy>update the current object</c>, rather than needing to <cl>reupload the object</c>.\n\nPlease note that if you are not <cg>Verified</c>, and you overwrite the object, the object will <cl>go back to pending</c>, meaning you will have to wait until a <cp>Reviewer</c> can <cg>accept</c> the object.\n\nPlease ensure that if you do overwrite the object, the object does not violate any <cr>upload rules</c>.", {280.F, 80.F});
    m_buttonMenu->addChildAtPosition(m_overwriteInfo, Anchor::Center, {0,65});

    m_buttonMenu->addChildAtPosition(overwriteLbl, Anchor::Center, {20,8});
    m_buttonMenu->addChildAtPosition(toggleBtn, Anchor::Center, {-65,8});

    m_previewBG = CCScale9Sprite::create("square02_small.png");
    m_previewBG->setOpacity(60);
    m_previewBG->setContentSize({ 295.F - 20.F, 82.F });
    auto previewLabel = CCLabelBMFont::create("Select an Object (For overwriting)", "goldFont.fnt");
    previewLabel->setScale(0.425F);
    m_previewBG->addChildAtPosition(previewLabel, Anchor::Top, {0,-8});

    if (auto editor = EditorUI::get()) {
        auto scrollLayer = ScrollLayerExt::create({ 0, 0, 275.0F, 280.0F }, true);
        scrollLayer->setContentSize({275.0F, 60.0F});
        scrollLayer->setAnchorPoint({0.5, 1.0});
        auto content = CCMenu::create();
        content->setScale(0.675F);
        content->setZOrder(2);
        content->setPositionX(20);
        content->registerWithTouchDispatcher();
        
        scrollLayer->m_contentLayer->addChild(content);
        scrollLayer->setTouchEnabled(true);
        CCArrayExt<CreateMenuItem*> customItems = editor->createCustomItems();
        int size = customItems.size() - 4;
        for (int i = 0; i < size; i++) {
            customItems[i]->setID(fmt::format("{}", i));
            if (i > 17) {
                customItems[i]->setEnabled(false);
            }
            content->addChild(customItems[i]);
        }
        m_previewBG->addChild(scrollLayer);
        content->setLayout(
            RowLayout::create()
                ->setAxisAlignment(AxisAlignment::Start)
                ->setCrossAxisAlignment(AxisAlignment::End)
                ->setAutoScale(true)
                ->setCrossAxisOverflow(false)
                ->setGap(5)
                ->setGrowCrossAxis(true)
        );
        content->setContentSize({400.0F, 400.0F});
        content->setAnchorPoint({0.5, 1.0});
        content->setPosition({137, 280});
        content->updateLayout();
        cocos::handleTouchPriority(scrollLayer);
        scrollLayer->moveToTop();
        scrollLayer->fixTouchPrio();
        scrollLayer->setCallbackMove([size, content]() {
            if (content == nullptr) return;
            for (int i = 0; i < size; i++) {
                if (auto child = typeinfo_cast<CreateMenuItem*>(content->getChildByID(fmt::format("{}", i)))) {
                    child->setEnabled(false);
                }
            }
        });
        scrollLayer->setCallbackEnd([size, content, scrollLayer]() {
            if (content == nullptr) return;
            for (int i = 0; i < size; i++) {
                if (auto child = typeinfo_cast<CreateMenuItem*>(content->getChildByID(fmt::format("{}", i)))) {
                    float contentYPos = scrollLayer->m_contentLayer->getPositionY();
                    float childYPos = (child->getPositionY());

                    child->setEnabled(!Utils::isInScrollSnapRange(contentYPos, childYPos));
                }
            }
            if (scrollLayer->m_contentLayer->getPositionY() > -220.F) {
                scrollLayer->m_contentLayer->setPositionY(Utils::getSnappedYPosition(scrollLayer->m_contentLayer->getPositionY(), 300)); // or 290
            }
        });
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
    m_listener.getFilter().cancel();
    auto notif = Notification::create("Updating Object...", NotificationIcon::Loading);
    notif->show();
    m_uploadListener.bind([this] (web::WebTask::Event* e) {
        if (web::WebResponse* value = e->getValue()) {
            auto jsonRes = value->json().unwrapOrDefault();
            if (Utils::notifError(jsonRes)) return;
            log::info("Overwrote object.");
            this->onClose(nullptr);
            Notification::create("Updated object!", NotificationIcon::Success)->show();
        } else if (web::WebProgress* progress = e->getProgress()) {
            // The request is still in progress...
        } else if (e->isCancelled()) {
            log::error("Request was cancelled.");
            this->onClose(nullptr);
        }
    });
    m_listener.bind([this, notif, token] (web::WebTask::Event* e) {
        if (web::WebResponse* value = e->getValue()) {
            auto jsonRes = value->json().unwrapOrDefault();
            if (Utils::notifError(jsonRes)) return;
            notif->hide();
            auto message = jsonRes.get("message");
            if (message.isOk()) {
                if (m_previewBG->isVisible()) { // assume they want to overwrite
                    if (auto editor = EditorUI::get()) {
                        if (auto gameManager = GameManager::sharedState()) {
                            if (editor->m_selectedObjectIndex < 0) { // genius robert!
                                m_object.objectString = gameManager->stringForCustomObject(editor->m_selectedObjectIndex);
                                web::WebRequest req = web::WebRequest();
                                req.userAgent(USER_AGENT);
                                auto myjson = matjson::Value();
                                myjson.set("token", token);
                                myjson.set("data", m_object.objectString);
                                req.header("Content-Type", "application/json");
                                req.bodyJSON(myjson);
                                m_uploadListener.setFilter(req.post(fmt::format("{}/objects/{}/overwrite", HOST_URL, m_object.id)));
                                return this->setVisible(false);
                            }
                        }
                    }
                }
                this->onClose(nullptr);
                Notification::create(message.unwrap().asString().unwrapOrDefault(), NotificationIcon::Success)->show();
            } else {
                log::error("Unknown response, expected message. {}", message.err());
                Notification::create("Got an unknown response, check logs for details.", NotificationIcon::Warning)->show();
                this->onClose(nullptr);
            }
            return;
        } else if (web::WebProgress* progress = e->getProgress()) {
            // The request is still in progress...
        } else if (e->isCancelled()) {
            log::error("Request was cancelled.");
            this->onClose(nullptr);
        }
    });
    if (m_objName != nullptr && m_objDesc->getString().length() > 0) {
        m_object.name = m_objName->getString();
    }
    m_object.description = "[No description provided]";
    if (m_objDesc != nullptr && m_objDesc->getString().length() > 0) {
        m_object.description = Utils::replaceAll(m_objDesc->getString(), "\\n", "\n");
    }
    web::WebRequest req = web::WebRequest();
    req.userAgent(USER_AGENT);
    auto myjson = matjson::Value();
    myjson.set("token", token);
    myjson.set("name", m_object.name);
    myjson.set("description", m_object.description);
    myjson.set("tags", m_object.tags);
    req.header("Content-Type", "application/json");
    req.bodyJSON(myjson);
    m_listener.setFilter(req.post(fmt::format("{}/objects/{}/update", HOST_URL, m_object.id)));
}
