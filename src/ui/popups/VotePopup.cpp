#include "VotePopup.hpp"
#include "../../config.hpp"

bool VotePopup::setup(CommentData obj, utils::MiniFunction<void()> callback) {
    m_data = obj;
    m_forceRefresh = callback;
    auto title = CCLabelBMFont::create("Vote", "bigFont.fnt");
    m_mainLayer->addChildAtPosition(title, Anchor::Top, {0, -20});
    auto likeSpr = CCSprite::createWithSpriteFrameName("GJ_likeBtn_001.png");
    likeSpr->setScale(1.2F);
    auto likeBtn = CCMenuItemSpriteExtra::create(
        likeSpr, this, menu_selector(VotePopup::onLike)
    );
    auto dislikeSpr = CCSprite::createWithSpriteFrameName("GJ_dislikeBtn_001.png");
    dislikeSpr->setScale(1.2F);
    auto dislikeBtn = CCMenuItemSpriteExtra::create(
        dislikeSpr, this, menu_selector(VotePopup::onDislike)
    );
    m_buttonMenu->addChildAtPosition(likeBtn, Anchor::Center, {-43, -11});
    m_buttonMenu->addChildAtPosition(dislikeBtn, Anchor::Center, {43, -11});
    return true;
}

void VotePopup::sendVote(bool like) {
    auto token = Mod::get()->getSettingValue<std::string>("token");
    m_listener.getFilter().cancel();
    this->setVisible(false);
    m_listener.bind([this, token] (web::WebTask::Event* e) {
        if (web::WebResponse* value = e->getValue()) {
            auto jsonRes = value->json().unwrapOrDefault();
            if (!jsonRes.is_object()) {
                log::error("Response isn't object.");
                this->onClose(nullptr);
                return;
            }
            auto isError = jsonRes.try_get<std::string>("error");
            if (isError) {
                Notification::create(isError->c_str(), NotificationIcon::Error)->show();
                this->onClose(nullptr);
                return;
            }
            auto message = jsonRes.try_get<std::string>("message");
            if (message) {
                m_forceRefresh();
                Notification::create(message->c_str(), NotificationIcon::Success)->show();
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
    web::WebRequest req = web::WebRequest();
    auto myjson = matjson::Value();
    myjson.set("token", token);
    myjson.set("like", (int)like);
    req.header("Content-Type", "application/json");
    req.bodyJSON(myjson);
    m_listener.setFilter(req.post(fmt::format("{}/objects/{}/comments/{}/vote", HOST_URL, m_data.objectID, m_data.id)));
}

void VotePopup::onLike(CCObject*) {
    VotePopup::sendVote(true);
}
void VotePopup::onDislike(CCObject*) {
    VotePopup::sendVote(false);
}
