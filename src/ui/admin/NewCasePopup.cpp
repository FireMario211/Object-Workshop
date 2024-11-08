#include "NewCasePopup.hpp"
#include <Geode/ui/TextInput.hpp>

// chat jippity
#include <iostream>
#include <ctime>
#include <iomanip>
#include <sstream>

std::string getCurrentDateFormatted() {
    // Get current time as time_t
    std::time_t t = std::time(nullptr);
    
    // Convert to local time (tm structure)
    std::tm* localTime = std::localtime(&t);
    
    // Create a string stream to format the date
    std::ostringstream oss;
    oss << std::setfill('0') << std::setw(2) << (localTime->tm_mon + 1) << "-"  // Month (tm_mon is 0-based, so add 1)
        << std::setw(2) << localTime->tm_mday << "-"                            // Day
        << (localTime->tm_year + 1900);                                         // Year (tm_year is years since 1900)
    
    return oss.str();
}

bool NewCasePopup::setup(UserData user, std::function<void()> callback) {
    this->setTitle("New Case");

    m_listener.bind([this, callback] (web::WebTask::Event* e) {
        if (web::WebResponse* value = e->getValue()) {
            auto jsonRes = value->json().unwrapOrDefault();
            if (!jsonRes.is_object()) return log::error("Response isn't object.");
            auto isError = jsonRes.try_get<std::string>("error");
            if (isError) {
                this->onClose(nullptr);
                return Notification::create(isError.value(), NotificationIcon::Error)->show();
            }
            auto message = jsonRes.try_get<std::string>("message");
            if (message) {
                Notification::create(message.value(), NotificationIcon::Success)->show();
                callback();
                this->onClose(nullptr);
            } else {
                log::error("Unknown response, expected message. {}", jsonRes.dump());
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

    auto expirationInput = TextInput::create(100.F, "Expiration Date...", "chatFont.fnt");
    expirationInput->setString(getCurrentDateFormatted());
    expirationInput->setMaxCharCount(11);
    expirationInput->setVisible(false);
    expirationInput->setFilter("0123456789-");
m_mainLayer->addChildAtPosition(expirationInput, Anchor::Center, {0, 10});

    auto caseLabel = CCLabelBMFont::create("Warning", "bigFont.fnt");
    caseLabel->setScale(0.5F);
    m_buttonMenu->addChildAtPosition(
        CCMenuItemExt::createSpriteExtraWithFrameName("edit_leftBtn_001.png", 1.0F, [this, caseLabel, expirationInput](CCObject*) {
            updateCaseIndex(false);
            caseLabel->setString(caseTypes[caseCurrentIndex].c_str());
            expirationInput->setVisible(caseCurrentIndex >= 1 && caseCurrentIndex <= 3);
        }),
        Anchor::Center,
        {-80, 50}
    );
    m_mainLayer->addChildAtPosition(caseLabel, Anchor::Center, {0, 50});
    m_buttonMenu->addChildAtPosition(
        CCMenuItemExt::createSpriteExtraWithFrameName("edit_rightBtn_001.png", 1.0F, [this, caseLabel, expirationInput](CCObject*) {
            updateCaseIndex(true);
            caseLabel->setString(caseTypes[caseCurrentIndex].c_str());
            expirationInput->setVisible(caseCurrentIndex >= 1 && caseCurrentIndex <= 3);
        }),
        Anchor::Center,
        {80, 50}
    );
    auto reasonInput = TextInput::create(200.F, "Reason...", "chatFont.fnt");
    reasonInput->setCommonFilter(CommonFilter::Any);
    m_mainLayer->addChildAtPosition(reasonInput, Anchor::Center, {0, -23});
    m_buttonMenu->addChildAtPosition(
        CCMenuItemExt::createSpriteExtra(ButtonSprite::create("Submit", "bigFont.fnt", "GJ_button_01.png", 0.5F), [this, user, reasonInput, expirationInput](CCObject*) {
            m_listener.getFilter().cancel();
            if (reasonInput->getString().empty()) return FLAlertLayer::create("Error", "You must <cg>enter a reason</c>!", "OK")->show();
            web::WebRequest req = web::WebRequest();
            req.userAgent(USER_AGENT);
            auto token = Mod::get()->getSettingValue<std::string>("token");
            auto myjson = matjson::Value();
            myjson.set("token", token);
            myjson.set("user", user.account_id);
            myjson.set("type", caseCurrentIndex);
            myjson.set("reason", reasonInput->getString());
            myjson.set("expiration", expirationInput->getString());
            req.header("Content-Type", "application/json");
            req.bodyJSON(myjson);
            m_listener.setFilter(req.post(fmt::format("{}/case/create", HOST_URL)));
            m_mainLayer->setVisible(false);
        }),
        Anchor::Bottom,
        { 0, 30 }
    );
    return true;
}

void NewCasePopup::onClose(CCObject* sender) {
    m_listener.getFilter().cancel();
    Popup::onClose(sender);
}
