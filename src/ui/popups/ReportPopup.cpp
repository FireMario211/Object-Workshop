#include "ReportPopup.hpp"
#include "../../config.hpp"
#include "Geode/ui/Popup.hpp"

bool ReportPopup::init(ObjectData obj) {
    if (!Popup::init(350.f, 100.f)) return false;
    m_object = obj;
    this->setTitle(fmt::format("Report {}", obj.name));
    m_reportInput = TextInput::create(300.0F, "Report Reason...", "chatFont.fnt");
    m_reportInput->setScale(0.8);
    m_reportInput->setMaxCharCount(100);
    m_reportInput->setCommonFilter(CommonFilter::Any);
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
                m_listener.cancel();
                web::WebRequest req = web::WebRequest();
                req.userAgent(USER_AGENT);
                auto certValid = Mod::get()->getSettingValue<bool>("cert-valid");
                if (!certValid) {
                    req.certVerification(certValid);
                }
                auto myjson = matjson::Value();
                myjson.set("token", token);
                myjson.set("reason", m_reportInput->getString());
                req.header("Content-Type", "application/json");
                req.bodyJSON(myjson);
                m_listener.spawn(
                    req.post(fmt::format("{}/objects/{}/report", HOST_URL, m_object.id)),
                    [this](web::WebResponse value) {
                        auto jsonRes = value.json().unwrapOrDefault();
                        if (Utils::notifError(jsonRes)) return;
                        auto message = jsonRes.get("message");
                        if (message.isOk()) {
                            Notification::create(message.unwrap().asString().unwrapOrDefault(), NotificationIcon::Success)->show();
                        } else {
                            log::error("Unknown response, expected message. {}", message.err());
                            Notification::create("Got an unknown response, check logs for details.", NotificationIcon::Warning)->show();
                        }
                        onClose(nullptr);
                    }
                );
            }
        },
        true,
        true
    );
}
