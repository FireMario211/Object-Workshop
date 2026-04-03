#include "WarningPopup.hpp"

using namespace geode::utils;

bool WarningPopup::init(CaseData caseData, std::function<void()> callback) {
    if (!Popup::init(260.f, 220.f)) return false;
    m_case = caseData;
    this->setTitle("Notice!");
    if (caseData.type == CaseType::Warning) {
        auto infoDesc = MDTextArea::create(fmt::format("## You have been <cy>warned</c> for the following reason:\n---\n{}\n\n---\n\nPlease be aware that repeated violations of the rules may result in more severe consequences.\n\n*By clicking **I acknowledge**, you confirm that you have read and understood this warning.*", m_case.reason), {220, 145});
        m_buttonMenu->addChildAtPosition(infoDesc, Anchor::Center);
        Build<CCLabelBMFont>::create(fmt::format("Case #{} - Warning #{}", m_case.id, m_case.number).c_str(), "chatFont.fnt")
            .color(0,0,0)
            .opacity(150)
            .anchorPoint(0.5, 0)
            .scale(0.55f)
            .parentAtPos(m_mainLayer, Anchor::Bottom, {0, 5});
        m_closeBtn->removeMeAndCleanup();
    } else {
        std::string banType;
        std::string willExpire;
        switch (caseData.type) {
            case CaseType::TBan:
                banType = "**<co>temporarily banned</c>** (Account)";
                break;
            case CaseType::TCommentBan:
                banType = "**<co>temporarily banned</c>** (Comment)";
                break;
            case CaseType::TUploadBan:
                banType = "**<co>temporarily banned</c>** (Upload)";
                break;
            case CaseType::Ban:
                banType = "**<cr>permanently banned</c>** (Account)";
                break;
            case CaseType::CommentBan:
                banType = "**<cr>permanently banned</c>** (Comment)";
                break;
            case CaseType::UploadBan:
                banType = "**<cr>permanently banned</c>** (Upload)";
                break;
            default:
                banType = "<cr>unknown</c>";
                break;
        }
        switch (caseData.type) {
            case CaseType::TBan:
            case CaseType::TCommentBan:
            case CaseType::TUploadBan:
                willExpire = fmt::format("The ban will expire on {}", m_case.expiration);
                break;
            default:
                willExpire = "The ban will not expire. You will need to appeal on Discord if you wish to be unbanned.";
                break;
        }
        auto infoDesc = MDTextArea::create(fmt::format("## You have been {} for the following reason:\n---\n{}\n\n---\n\n{}\n\n\n*By clicking **I acknowledge**, you confirm that you have read this notice.*", banType, m_case.reason, willExpire), {220, 145});
        m_buttonMenu->addChildAtPosition(infoDesc, Anchor::Center);
        Build<CCLabelBMFont>::create(fmt::format("Case #{} - Warning #{}", m_case.id, m_case.number).c_str(), "chatFont.fnt")
            .color(0,0,0)
            .opacity(150)
            .anchorPoint(0.5, 0)
            .scale(0.55f)
            .parentAtPos(m_mainLayer, Anchor::Bottom, {0, 5});
        m_closeBtn->removeMeAndCleanup();
    }
    
    Build<ButtonSprite>::create("I acknowledge", "bigFont.fnt", "GJ_button_01.png").scale(0.5F).intoMenuItem([this, callback]() {
        m_mainLayer->setVisible(false);
        m_listener.cancel();
        auto token = Mod::get()->getSettingValue<std::string>("token");
        web::WebRequest req = web::WebRequest();
        req.userAgent(USER_AGENT);
        auto certValid = Mod::get()->getSettingValue<bool>("cert-valid");
        if (!certValid) {
            req.certVerification(certValid);
        }
        auto myjson = matjson::Value();
        myjson.set("token", token);
        req.header("Content-Type", "application/json");
        req.bodyJSON(myjson);
        m_listener.spawn(
            req.post(fmt::format("{}/case/{}/ack", HOST_URL, m_case.id)),
            [this, callback](web::WebResponse value) {
                auto jsonRes = value.json().unwrapOrDefault();
                Utils::notifError(jsonRes);
                this->onClose(nullptr);
                callback();
            }
        );

    }).parentAtPos(m_buttonMenu, Anchor::Bottom, {0, 25});
    return true;
}
