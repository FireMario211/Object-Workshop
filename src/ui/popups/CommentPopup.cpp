#include "CommentPopup.hpp"
#include "../../config.hpp"
#include "../../utils.hpp"

bool CommentPopup::setup(ObjectData obj, std::function<void()> callback) {
    this->setID("CommentPopup"_spr);
    auto winSize = CCDirector::sharedDirector()->getWinSize();
    m_object = obj;
    m_submitCallback = callback;
    auto titleLbl = CCLabelBMFont::create("Add Comment", "bigFont.fnt");
    titleLbl->setScale(0.6F);
    m_closeBtn->removeMeAndCleanup();
    m_mainLayer->addChildAtPosition(titleLbl, Anchor::Top, {0, -15});
    std::string rules = "1. Do <cr>not</c> spam.\n2. Do <cr>not</c> harass other players.\n3. Do <cr>not</c> post inappropriate or controversial content.\n4. Do <cr>not</c> try to bypass these rules.\n<cy>These rules and their enforcement are entirely at Staff's discretion.</c>";
    m_buttonMenu->addChildAtPosition(InfoAlertButton::create("Comment Rules", rules, 1.0F), Anchor::TopLeft, {3, -3});
    m_buttonMenu->addChildAtPosition(
        CCMenuItemSpriteExtra::create(
            ButtonSprite::create("Cancel"),
            this,
            menu_selector(CommentPopup::onClose)
        ), Anchor::Bottom, {-60, 25}
    );
    m_buttonMenu->addChildAtPosition(
        CCMenuItemSpriteExtra::create(
            ButtonSprite::create("Submit"),
            this,
            menu_selector(CommentPopup::onSubmit)
        ), Anchor::Bottom, {60, 25}
    );
    auto commentBG = CCScale9Sprite::create("square02b_001.png");
    commentBG->setColor({0,0,0});
    commentBG->setOpacity(100);
    commentBG->setContentSize({360, 60});
    // most undocumented thing ever, why does it have to be 0x0 to allow new lines!? i tried chatFont.fnt before but apparently that keeps the CCLabelBMFont!
    /*auto textInput = CCTextInputNode::create(360.F, 50.F, "Insert comment", "Thonburi", 24, 0x0);
    textInput->setAllowedChars(" abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789.,-!?:;)(/\\\"\'`*=+- _%[]<>|@&^#{}%$~");
    textInput->setMaxLabelLength(100);
    textInput->setLabelPlaceholderColor({0xc8, 0xc8, 0xc8});
    textInput->setDelegate(this);
    textInput->addTextArea(TextArea::create("","chatFont.fnt",1.0F,295.F,{0.5, 0.5},20.F,true));*/
    m_charCountLabel = cocos2d::CCLabelBMFont::create("100","chatFont.fnt");
    m_charCountLabel->setAnchorPoint({1.0, 0.5});
    m_charCountLabel->setColor({0,0,0});
    m_charCountLabel->setOpacity(125);
    m_mainLayer->addChildAtPosition(m_charCountLabel, Anchor::TopRight, {-10, -15});
    //m_mainLayer->addChildAtPosition(commentBG, Anchor::Center, {0, 9});
    //m_buttonMenu->addChildAtPosition(textInput, Anchor::Center, {0, 9});
    m_inputNode = TextInputNode::create("Insert comment");
    m_inputNode->setUpdateCallback([this](std::string text) {
        if (m_closed) return;
        m_descText = text;
        updateCharCountLabel();
    });
    m_mainLayer->addChildAtPosition(m_inputNode, Anchor::Center, {0, 9});
    m_buttonMenu->addChildAtPosition(m_inputNode->getInput(), Anchor::Center, {0, 9});
    /*m_mainLayer->setPositionY(winSize.height * 0.75F);
    m_mainLayer->updateLayout();*/ 
    // this causes text input to break for SOME reason
    return true;
}

void CommentPopup::updateCharCountLabel() {
    if (m_closed) return;
    if (m_charCountLabel != nullptr) {
        int amount = 100 - m_descText.length();
        if (amount < 0) {
            m_charCountLabel->setColor({255,0,0});
            m_charCountLabel->setString("Too much!");
        } else {
            m_charCountLabel->setColor({0,0,0});
            m_charCountLabel->setString(std::to_string(100 - m_descText.length()).c_str());
        }
    }
}

void CommentPopup::onClose(CCObject* sender) {
    m_closed = true;
    m_inputNode->cancel = true;
    Popup::onClose(sender);
}

void CommentPopup::onSubmit(CCObject*) {
    if (m_descText.empty()) return;
    if (m_closed) return;
    if (m_descText.length() > 100) return FLAlertLayer::create("what", "you arent supposed to be above 100 characters...", "WHAT")->show();
    m_mainLayer->setVisible(false);
    auto token = Mod::get()->getSettingValue<std::string>("token");
    m_listener.getFilter().cancel();
    m_listener.bind([this] (web::WebTask::Event* e) {
        if (web::WebResponse* value = e->getValue()) {
            auto jsonRes = value->json().unwrapOrDefault();
            if (Utils::notifError(jsonRes)) {
                return this->onClose(nullptr);
            }
            auto message = jsonRes.get("message");
            if (message.isOk()) {
                Notification::create(message.unwrap().asString().unwrapOrDefault(), NotificationIcon::Success)->show();
                m_submitCallback();
                this->onClose(nullptr);
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
    web::WebRequest req = web::WebRequest();
    req.userAgent(USER_AGENT);
    auto myjson = matjson::Value();
    myjson.set("token", token);

    m_descText = Utils::replaceAll(m_descText, "\\n", "\n");
    myjson.set("data", m_descText);
    req.header("Content-Type", "application/json");
    req.bodyJSON(myjson);
    m_listener.setFilter(req.post(fmt::format("{}/objects/{}/comment", HOST_URL, m_object.id)));
}
