#include "ReportPopup.hpp"
#include "../config.hpp"

bool ReportPopup::setup(ObjectData obj) {
    m_object = obj;
    this->setTitle(fmt::format("Report {}", obj.name));
    m_reportInput = TextInput::create(300.0F, "Report Reason...", "chatFont.fnt");
    m_reportInput->setScale(0.8);
    m_reportInput->setMaxCharCount(100);
    m_mainLayer->addChildAtPosition(m_reportInput, Anchor::Center, {0, 8});
    
    auto reportSpr = ButtonSprite::create("Report", "bigFont.fnt", "GJ_button_01.png");
    reportSpr->setScale(0.8F);
    auto reportBtn = CCMenuItemSpriteExtra::create(
        reportSpr,
        this,
        menu_selector(ReportPopup::onReportBtn)
    );
    m_buttonMenu->addChildAtPosition(reportBtn, Anchor::Bottom, {0, 25});
    return true;
}

void ReportPopup::onReportBtn(CCObject*) {
    if (m_reportInput->getString().empty()) return FLAlertLayer::create("Error", "Please enter in the <cy>report reason</c>!", "OK")->show();
    geode::createQuickPopup(
        "Warning",
        "Are you sure you want to <cy>report this object</c>?\n\nPlease make sure this object <cr>violates the guidelines</c> before reporting. Any misuse of this button will result in <cr>a ban</c>",
        "No",
        "Yes",
        [this](auto, bool btn2) {
            auto token = Mod::get()->getSettingValue<std::string>("token");
            if (btn2) {
                m_listener.getFilter().cancel();
                m_listener.bind([this] (web::WebTask::Event* e) {
                    if (web::WebResponse* value = e->getValue()) {
                        auto jsonRes = value->json().unwrapOrDefault();
                        if (!jsonRes.is_object()) return log::error("Response isn't object.");
                        auto jsonObj = jsonRes.as_object();
                        auto isError = jsonRes.try_get<std::string>("error");
                        if (isError) return Notification::create(isError->c_str(), NotificationIcon::Error)->show();
                        Notification::create(jsonRes.get<std::string>("message").c_str(), NotificationIcon::Success)->show();
                        onClose(nullptr);
                        return;
                    } else if (web::WebProgress* progress = e->getProgress()) {
                        // The request is still in progress...
                    } else if (e->isCancelled()) {
                        log::error("Request was cancelled.");
                    }
                });
                web::WebRequest req = web::WebRequest();
                auto myjson = matjson::Value();
                myjson.set("token", token);
                myjson.set("reason", m_reportInput->getString());
                req.header("Content-Type", "application/json");
                req.bodyJSON(myjson);
                m_listener.setFilter(req.post(fmt::format("{}/objects/{}/report", HOST_URL, m_object.id)));
            }
        },
        true,
        true
    );
}
