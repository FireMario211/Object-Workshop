#include "ReportPopup.hpp"
#include "../../config.hpp"
#include "Geode/ui/Popup.hpp"
#include "../ObjectWorkshop.hpp"

//1. Do not <cy>upload spam, duplicate, or useless objects.</c>\n2. Do not upload <cy>any stolen art</c>, objects that <cy>violate copyright</c>, or objects <cy>without the creators permission.</c>\n3. Do not upload objects that relate to anything that is <cy>inappropriate, explicit, sexual, or violent.</c>\n4. Use an <cy>appropriate name, and tags</c> when uploading.\n\nBreaking these rules will result in a <cr>temporary ban</c>, and possibly a <cr>permanent one</c> if <cy>too severe or repeated.</c>\n\nBy checking the box on **I agree**, you agree to these rules.", {2
static const std::vector<std::string> s_reasons = {
    "I don't like it.", // this does nothing lol
    "Spam or useless. (Just one block)",
    "A duplicate of an object that had been uploaded before.",
    "The custom object is an entire level.",
    "The custom object is copying another creator's work without their permission.",
    "Contains swears in object itself, name, or description.",
    "Claims to be/is a crash trigger.",
    "Uses img2gd",
    "Inappropriate, explicit, sexual, or violent.",
    "The object lags the game on purpose.",
    "Custom Reason."
};

//
//Do not upload objects other creators have already uploaded to the workshop.
//Do not upload entire levels to the workshop.
//Adding swears to the object, name or description of the object is against the rules, as Geometry Dash is a game rated E for everyone
//Do not upload objects that claim to be crash triggers.
//Do not upload crash triggers.
//Do not upload objects that relate to anything that is inappropriate, explicit, sexual, or violent.
//Your recent upload "skull" was rejected due to copying another creator's work without their permission.

bool ReportPopup::init(ObjectData obj, ReportActionType type) {
    if (!Popup::init(350.f, 200.f)) return false;
    m_object = obj;
    m_type = type;
    if (type == ReportActionType::Report) {
        this->setTitle(fmt::format("Report {}", obj.name));
    } else if (type == ReportActionType::Appeal) {
        this->setTitle(fmt::format("Appeal {}", obj.name));
    } else {
        this->setTitle(fmt::format("Reject {}", obj.name));
    }
    if (type != ReportActionType::Appeal) {
        m_combo = ComboComponentNode::create(300, s_reasons, [this](bool opened) {
            if (opened) m_reportInput->setVisible(false);
        }, [this, obj](std::string str, size_t i) {
            m_selectedIndex = i;
            if (str == s_reasons.back()) {
                m_reportInput->setString("");
                m_reportInput->setVisible(true);
            } else {
                std::string reportReason = "No reason provided.";
                switch (i) {
                    case 1:
                        reportReason = "Do not upload spam, duplicate, or useless objects.";
                        break;
                    case 2:
                        reportReason = "Do not upload objects other creators have already uploaded to the workshop.";
                        break;
                    case 3:
                        reportReason = "Do not upload entire levels to the workshop.";
                        break;
                    case 4:
                        reportReason = fmt::format("The object \"{}\" was rejected due to copying another creator's work without their permission.", obj.name);
                        break;
                    case 5:
                        reportReason = "Adding swears to the object, name or description of the object is against the rules, as Geometry Dash is a game rated E for everyone.";
                        break;
                    case 6:
                        reportReason = "Do not upload objects that claim to be, or are crash triggers.";
                        break;
                    case 7:
                        reportReason = "Do not upload art/backgrounds made in img2gd.";
                        break;
                    case 8:
                        reportReason = "Do not upload objects that relate to anything that is inappropriate, explicit, sexual, or violent.";
                        break;
                    case 9:
                        reportReason = "Do not upload objects that lag the game.";
                        break;
                }
                m_reportInput->setString(reportReason);
                m_reportInput->setVisible(false);
            }
        });
        m_buttonMenu->addChildAtPosition(m_combo, Anchor::Center, {0, 50});
    } else {

    }
    m_reportInput = TextInput::create(300.0F, (type == ReportActionType::Report) ? "Report Reason..." : (type == ReportActionType::Appeal) ? "Appeal Reason..." : "Reject Reason...", "chatFont.fnt");
    m_reportInput->setScale(0.8);
    if (type == ReportActionType::Report) {
        m_reportInput->setMaxCharCount(100);
    } else {
        m_reportInput->setMaxCharCount(500);
    }
    m_reportInput->setCommonFilter(CommonFilter::Any);
    m_reportInput->setVisible(false);
    m_mainLayer->addChildAtPosition(m_reportInput, Anchor::Center, {0, 20});
    
    std::string btnTitle = "Report";
    if (type == ReportActionType::Review) {
        btnTitle = "Reject";
        //Build<CCLabelBMFont>::create("Force Reject", "bigFont.fnt").anchorPoint(0, 0.5).scale(0.45f).parentAtPos(m_buttonMenu, Anchor::BottomLeft, {45, 23});
        Build<CCMenuItemToggler>::createToggle(Build<CCSprite>::createSpriteName("GJ_checkOff_001.png").scale(0.65f).collect(), Build<CCSprite>::createSpriteName("GJ_checkOn_001.png").scale(0.65f).collect(), [this](auto){}).store(m_forceReject).parentAtPos(m_buttonMenu, Anchor::BottomLeft, {30, 23});
    } else if (type == ReportActionType::ReviewAndCase) {
        btnTitle = "Reject & Moderate";
    } else if (type == ReportActionType::Appeal) {
        btnTitle = "Appeal";
    }
    auto reportSpr = ButtonSprite::create(btnTitle.c_str(), "bigFont.fnt", "GJ_button_01.png");
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
    if (m_reportInput->getString().empty()) return FLAlertLayer::create("Error", "Please enter in the <cy>reason</c>!", "OK")->show();
    auto token = Mod::get()->getSettingValue<std::string>("token");
    if (m_type != ReportActionType::Report && m_type != ReportActionType::Appeal) {
        geode::createQuickPopup("Warning", "Are you sure you want to <cy>reject this object</c>?", "No", "Yes", [this, token](auto, bool btn2) {
            if (btn2) {
                web::WebRequest req = web::WebRequest();
                req.userAgent(USER_AGENT);
                auto certValid = Mod::get()->getSettingValue<bool>("cert-valid");
                if (!certValid) {
                    req.certVerification(certValid);
                }
                auto myjson = matjson::Value();
                myjson.set("token", token);
                myjson.set("reason", ZStringView(m_reportInput->getString()));
                myjson.set("mod", m_type == ReportActionType::ReviewAndCase ? 1 : 0);
                myjson.set("force", m_type == ReportActionType::Review && (m_forceReject && m_forceReject->isToggled()) ? 1 : 0);
                req.header("Content-Type", "application/json");
                req.bodyJSON(myjson);
                m_listener.spawn(
                    req.post(fmt::format("{}/objects/{}/reject", HOST_URL, m_object.id)),
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
                        this->onClose(nullptr);
                        m_callback();
                    }
                );
            }
        }, true, true);
    } else {
        std::string desc = "Are you sure you want to <cy>report this object</c>?\n\nPlease make sure this object <cr>violates the guidelines</c> before reporting. Any misuse of this button will result in <cr>a ban</c>";
        if (m_type != ReportActionType::Report) {
            desc = "Are you sure you want to <cg>appeal this object</c>?\n\nYou will receive a punishment if the object still violates guidelines if you submit an appeal."
        }
        geode::createQuickPopup(
            "Warning",
            desc,
            "No",
            "Yes",
            [this, token](auto, bool btn2) {
                if (btn2) {
                    if (ZStringView(m_reportInput->getString()) == "No reason provided.") {
                        Notification::create("Reported.", NotificationIcon::Success)->show();
                        onClose(nullptr);
                        return;
                    }
                    m_listener.cancel();
                    web::WebRequest req = web::WebRequest();
                    req.userAgent(USER_AGENT);
                    auto certValid = Mod::get()->getSettingValue<bool>("cert-valid");
                    if (!certValid) {
                        req.certVerification(certValid);
                    }
                    auto myjson = matjson::Value();
                    myjson.set("token", token);
                    myjson.set("reason", ZStringView(m_reportInput->getString()));
                    req.header("Content-Type", "application/json");
                    req.bodyJSON(myjson);
                    m_listener.spawn(
                        req.post(fmt::format("{}/objects/{}/{}", HOST_URL, m_object.id, m_type == ReportActionType::Appeal ? "appeal" : "report")),
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
}
