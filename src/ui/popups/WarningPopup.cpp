#include "WarningPopup.hpp"

bool WarningPopup::setup(CaseData caseData, std::function<void()> callback) {
    m_case = caseData;
    this->setTitle("Notice!");
    auto infoDesc = MDTextArea::create(fmt::format("## You have been <cy>warned</c> for the following reason:\n---\n{}\n\n---\n\nPlease be aware that repeated violations of the rules may result in more severe consequences.\n\n*By clicking **I acknowledge**, you confirm that you have read and understood this warning.*", m_case.reason), {220, 145});
    m_buttonMenu->addChildAtPosition(infoDesc, Anchor::Center);
    auto footer = CCLabelBMFont::create(fmt::format("Case #{} - Warning #{}", m_case.id, m_case.number).c_str(), "chatFont.fnt");
    footer->setColor({0,0,0});
    footer->setOpacity(150);
    footer->setAnchorPoint({0.5, 0});
    footer->setScale(0.55F);
    m_mainLayer->addChildAtPosition(footer, Anchor::Bottom, {0, 5});
    m_closeBtn->removeMeAndCleanup();
    auto ackSpr = ButtonSprite::create("I acknowledge", "bigFont.fnt", "GJ_button_01.png");
    ackSpr->setScale(0.5F);

    m_listener.bind([this, callback] (web::WebTask::Event* e) {
        if (web::WebResponse* value = e->getValue()) {
            auto jsonRes = value->json().unwrapOrDefault();
            if (!jsonRes.is_object()) {
                this->onClose(nullptr);
                callback();
                return log::error("Response isn't object.");
            }
            auto isError = jsonRes.try_get<std::string>("error");
            if (isError) {
                Notification::create(isError.value(), NotificationIcon::Error)->show();
                this->onClose(nullptr);
                return callback();
            }
            this->onClose(nullptr);
            callback();
            return;
        } else if (web::WebProgress* progress = e->getProgress()) {
            // The request is still in progress...
        } else if (e->isCancelled()) {
            log::error("Request was cancelled.");
            this->onClose(nullptr);
            callback();
        }
    });

    auto ackBtn = CCMenuItemExt::createSpriteExtra(ackSpr, [this](CCObject* sender) {
        m_mainLayer->setVisible(false);
        m_listener.getFilter().cancel();
        auto token = Mod::get()->getSettingValue<std::string>("token");
        web::WebRequest req = web::WebRequest();
        req.userAgent(USER_AGENT);
        auto myjson = matjson::Value();
        myjson.set("token", token);
        req.header("Content-Type", "application/json");
        req.bodyJSON(myjson);
        m_listener.setFilter(req.post(fmt::format("{}/case/{}/ack", HOST_URL, m_case.id)));
    });

    m_buttonMenu->addChildAtPosition(ackBtn, Anchor::Bottom, {0, 25});

    return true;
}
