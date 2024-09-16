#include "CommentCell.hpp"
#include "../config.hpp"
#include "../utils.hpp"
#include "../ui/popups/VotePopup.hpp"

bool OWCommentCell::init(CommentData data, ObjectData obj, UserData user, utils::MiniFunction<void()> callback) {
    if (!CCScale9Sprite::init()) return false;
    m_data = data;
    m_user = user;
    m_forceRefresh = callback;
    this->setAnchorPoint({ 0.5, 0.5 });
    this->setContentSize({ 137.0f, 40.0f });

    auto menu = CCMenu::create();
    menu->setAnchorPoint({0.5, 0.5});
    menu->setContentSize(this->getContentSize());

    //square02c_001-uhd.png
    //square02b_001.png
    //square02_small.png
    auto bgSpr = cocos2d::extension::CCScale9Sprite::create("square02b_001.png");
    bgSpr->setContentSize(this->getContentSize());
    bgSpr->setColor({95, 53, 31});
    auto bgSprBehind = cocos2d::extension::CCScale9Sprite::create("square02b_001.png");
    bgSprBehind->setContentSize({this->getContentWidth() + 3.5F, this->getContentHeight() + 3.5F});
    bgSprBehind->setColor({86, 48, 29});
    if (data.pinned) {
        bgSprBehind->setColor({ 0, 195, 255 });
        bgSpr->setColor({ 68, 85, 90 });
    }

    auto authorName = CCLabelBMFont::create(data.accountName.c_str(), "goldFont.fnt");
    authorName->limitLabelWidth(80.0F, 0.45F, 0.1F); // 0.425
    authorName->setAnchorPoint({0, 0.5});
    
    // icon,playerColor,playerColor2,playerColorGlow,glow
    if (auto gm = GameManager::sharedState()) {
        SimplePlayer* pIcon;
        if (m_data.icon.size() != 5) {
            pIcon = SimplePlayer::create(1);
        } else {
            pIcon = SimplePlayer::create(m_data.icon[0].as_int());
            pIcon->setColor(gm->colorForIdx(m_data.icon[1].as_int()));
            pIcon->setSecondColor(gm->colorForIdx(m_data.icon[2].as_int()));
            pIcon->setGlowOutline(gm->colorForIdx(m_data.icon[3].as_int()));
            pIcon->m_hasGlowOutline = m_data.icon[4].as_int() == 1;
            pIcon->updateColors();
        }
        pIcon->setScale(0.45f);
        bgSpr->addChildAtPosition(pIcon, Anchor::TopLeft, {11, -10});
    }
    //m_data.content = "AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA";
    auto commentText = TextArea::create(
        m_data.content, "chatFont.fnt",
        Utils::calculateScale(m_data.content, 16, 100, 0.5F, 0.3F),
        120.F, { 0.0f, 0.5f }, // 1.0
        Utils::calculateScale(m_data.content, 16, 100, 10.F, 5.F),
        true);
	commentText->setAnchorPoint({ 0.0f, 0.5f });
    if (data.role == 2) {
        commentText->colorAllCharactersTo({0, 255, 0});
    }
    if (data.role == 3) {
        commentText->colorAllCharactersTo({221, 151, 254});
    }
    if (data.accountID == obj.authorAccId) {
        commentText->colorAllCharactersTo({255, 255, 0});
    }
    

    auto timeAgo = CCLabelBMFont::create(m_data.timestamp.c_str(), "chatFont.fnt");
    timeAgo->setColor({0,0,0});
    timeAgo->setAnchorPoint({1, 0.5});
    timeAgo->setScale(0.35F);
    timeAgo->setOpacity(125);

    auto voteSpr = CCSprite::createWithSpriteFrameName((m_data.likes >= 0) ? "GJ_likesIcon_001.png" : "GJ_dislikesIcon_001.png");
    voteSpr->setScale(0.5F);
    auto voteBtn = CCMenuItemSpriteExtra::create(voteSpr, this, menu_selector(OWCommentCell::onVote));

    auto deleteSpr = CCSprite::createWithSpriteFrameName("GJ_deleteIcon_001.png");
    deleteSpr->setScale(0.45F);
    auto deleteBtn = CCMenuItemSpriteExtra::create(deleteSpr, this, menu_selector(OWCommentCell::onDelete));
    
    auto pinSpr = CCSprite::createWithSpriteFrameName("GJ_starsIcon_001.png");
    pinSpr->setScale(0.5F);
    auto pinBtn = CCMenuItemSpriteExtra::create(pinSpr, this, menu_selector(OWCommentCell::onPin));

    auto votesLabel = CCLabelBMFont::create(std::to_string(m_data.likes).c_str(), "bigFont.fnt");
    votesLabel->setScale(0.35F);

    bgSpr->addChildAtPosition(commentText, Anchor::Left, {4, -2});
    bgSpr->addChildAtPosition(authorName, Anchor::TopLeft, {22, -9});
    bgSpr->addChildAtPosition(timeAgo, Anchor::BottomRight, {-3, 5});

    menu->addChildAtPosition(voteBtn, Anchor::TopRight, {-25, -8});
    if (m_user.role >= 3 || m_user.account_id == obj.authorAccId || m_user.account_id == data.accountID) {
        menu->addChildAtPosition(deleteBtn, Anchor::TopRight, {-40, -8.5});
    }
    if (m_user.role >= 3 || m_user.account_id == obj.authorAccId) {
        menu->addChildAtPosition(pinBtn, Anchor::TopRight, {-53, -8.5});
    }

    bgSpr->addChildAtPosition(votesLabel, Anchor::TopRight, {-9, -8});
    if (data.pinned) {
        this->addChildAtPosition(bgSprBehind, Anchor::Center);
    }
    this->addChildAtPosition(bgSpr, Anchor::Center);
    this->addChildAtPosition(menu, Anchor::Center);
    return true;
}

void OWCommentCell::onVote(CCObject*) {
    VotePopup::create(m_data, m_forceRefresh)->show();
}
void OWCommentCell::onPin(CCObject*) {
    geode::createQuickPopup(
        "Note",
        "Are you sure you want to <cy>pin this comment</c>?\n\nThis will <cr>replace the current pinned comment</c> if there is any.",
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
                        auto message = jsonRes.try_get<std::string>("message");
                        if (message) {
                            Notification::create(message->c_str(), NotificationIcon::Success)->show();
                            m_forceRefresh();
                        } else {
                            log::error("Unknown response, expected message. {}", jsonRes.dump());
                            Notification::create("Got an unknown response, check logs for details.", NotificationIcon::Warning)->show();
                        }
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
                req.header("Content-Type", "application/json");
                req.bodyJSON(myjson);
                m_listener.setFilter(req.post(fmt::format("{}/objects/{}/comments/{}/pin", HOST_URL, m_data.objectID, m_data.id)));
            }
        },
        true,
        true
    );
}
void OWCommentCell::onDelete(CCObject*) {
    geode::createQuickPopup(
        "Note",
        "Are you sure you want to <cr>delete this comment</c>?\n\nYou cannot <cy>undo this action</c>!",
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
                        auto message = jsonRes.try_get<std::string>("message");
                        if (message) {
                            Notification::create(message->c_str(), NotificationIcon::Success)->show();
                            m_forceRefresh();
                        } else {
                            log::error("Unknown response, expected message. {}", jsonRes.dump());
                            Notification::create("Got an unknown response, check logs for details.", NotificationIcon::Warning)->show();
                        }
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
                req.header("Content-Type", "application/json");
                req.bodyJSON(myjson);
                m_listener.setFilter(req.post(fmt::format("{}/objects/{}/comments/{}/delete", HOST_URL, m_data.objectID, m_data.id)));
            }
        },
        true,
        true
    );
}

OWCommentCell* OWCommentCell::create(CommentData data, ObjectData obj, UserData user, utils::MiniFunction<void()> callback) {
    auto pRet = new OWCommentCell();
    if (pRet) {
        if (pRet->init(data, obj, user, callback)) {
            pRet->autorelease();
            return pRet;
        }
    }
    CC_SAFE_DELETE(pRet);
    return nullptr;
};