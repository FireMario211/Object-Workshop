#include "EditPopup.hpp"
#include "../utils.hpp"
#include "../config.hpp"
#include "FiltersPopup.hpp"

bool EditPopup::setup(ObjectData obj, std::unordered_set<std::string> availableTags) {
    m_availableTags = availableTags;
    m_object = obj;
    this->setTitle(fmt::format("Edit {}", obj.name));
    m_objName = TextInput::create(300.0F, "Object Name", "bigFont.fnt");
    m_objName->setScale(0.8);
    m_objName->setMaxCharCount(64);
    m_objName->setString(obj.name);
    m_mainLayer->addChildAtPosition(m_objName, Anchor::Center, {0, 35});
    
    auto textArea = TextArea::create("", "chatFont.fnt", 1.0F, 270.0F, {0.5, 0.5}, 20.0F, true);
    //             TextArea::create(&local_64,"chatFont.fnt",,0x439d8000,this_03,0x41a00000,1);
    m_objDesc = TextInput::create(270.0F, "Description [Optional]", "chatFont.fnt");
    m_objDesc->getInputNode()->addTextArea(textArea);
    m_objDesc->getBGSprite()->setContentSize({520.0F, 100.0F});
    m_objDesc->setMaxCharCount(300);
    m_objDesc->getInputNode()->m_cursor->setOpacity(0);
    m_objDesc->setCommonFilter(CommonFilter::Any);
    m_mainLayer->addChildAtPosition(m_objDesc, Anchor::Center, {0, -5});
    
    m_objDesc->setString(obj.description);
    m_objDesc->getInputNode()->m_placeholderLabel->setOpacity((obj.description.length() == 0) ? 255 : 0);
    textArea->setScale(Utils::calculateScale(obj.description, 50, 300, 1.0F, 0.35F));
    textArea->m_width = 220.0F / Utils::calculateScale(obj.description, 50, 300, 1.0F, 0.32F);
    textArea->setString(obj.description);
    m_objDesc->setCallback(
        [this, textArea](std::string p0) {
            m_objDesc->getInputNode()->m_placeholderLabel->setOpacity((p0.length() == 0) ? 255 : 0);
            textArea->setScale(Utils::calculateScale(p0, 50, 300, 1.0F, 0.35F));
            textArea->m_width = 220.0F / Utils::calculateScale(p0, 50, 300, 1.0F, 0.32F);
            textArea->setString(p0);
        }
    );

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
    auto filterBtn = CCMenuItemSpriteExtra::create(
        filterSpr,
        this,
        menu_selector(EditPopup::onFilterBtn)
    );
    m_buttonMenu->addChildAtPosition(filterBtn, Anchor::Bottom, {-50, 28});

    auto uploadSpr = ButtonSprite::create("Update", "bigFont.fnt", "GJ_button_01.png");
    uploadSpr->setScale(0.8F);
    auto uploadBtn = CCMenuItemSpriteExtra::create(
        uploadSpr,
        this,
        menu_selector(EditPopup::onUpdateBtn)
    );
    m_buttonMenu->addChildAtPosition(uploadBtn, Anchor::Bottom, {30, 28});
    return true;
}

void EditPopup::onFilterBtn(CCObject*) {
    FiltersPopup::create(m_availableTags, m_object.tags, true, [this](std::unordered_set<std::string> selectedTags) {
        m_object.tags = selectedTags;
    })->show();
}
void EditPopup::onUpdateBtn(CCObject*) {
    auto token = Mod::get()->getSettingValue<std::string>("token");
    m_listener.getFilter().cancel();
    auto notif = Notification::create("Updating Object...", NotificationIcon::Loading);
    notif->show();
    m_listener.bind([this, notif] (web::WebTask::Event* e) {
        if (web::WebResponse* value = e->getValue()) {
            auto jsonRes = value->json().unwrapOrDefault();
            if (!jsonRes.is_object()) return log::error("Response isn't object.");
            auto jsonObj = jsonRes.as_object();
            auto isError = jsonRes.try_get<std::string>("error");
            if (isError) return Notification::create(isError->c_str(), NotificationIcon::Error)->show();
            notif->hide();
            Notification::create(jsonRes.get<std::string>("message").c_str(), NotificationIcon::Success)->show();
            this->onClose(nullptr);
            return;
        } else if (web::WebProgress* progress = e->getProgress()) {
            // The request is still in progress...
        } else if (e->isCancelled()) {
            log::error("Request was cancelled.");
        }
    });
    m_object.description = "[No description provided]";
    if (m_objDesc != nullptr && m_objDesc->getString().length() > 0) {
        m_object.description = m_objDesc->getString();
    }
    web::WebRequest req = web::WebRequest();
    auto myjson = matjson::Value();
    myjson.set("token", token);
    myjson.set("name", m_object.name);
    myjson.set("description", m_object.description);
    myjson.set("tags", m_object.tags);
    req.header("Content-Type", "application/json");
    req.bodyJSON(myjson);
    m_listener.setFilter(req.post(fmt::format("{}/objects/{}/update", HOST_URL, m_object.id)));
}
