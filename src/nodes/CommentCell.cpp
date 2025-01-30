#include "CommentCell.hpp"
#include "../config.hpp"
#include "../utils.hpp"
#include "../ui/popups/VotePopup.hpp"
#include "../ui/popups/CommentsPopup.hpp"
#include "../ui/popups/ObjectPopup.hpp"

bool OWCommentCell::init(CommentData data, ObjectData obj, UserData user, std::function<void()> callback) {
    if (!CCScale9Sprite::init()) return false;
    m_data = data;
    m_user = user;
    m_forceRefresh = callback;
    this->setAnchorPoint({ 0.5, 0.5 });
    //this->setContentSize({ 137.0f, 40.0f });
    this->setContentSize({ 200.0f, 40.0f });
    this->setID("comment/cell"_spr);

    auto menu = CCMenu::create();
    menu->setAnchorPoint({0.5, 0.5});
    menu->setContentSize(this->getContentSize());

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
    auto authorBtn = CCMenuItemExt::createSpriteExtra(authorName, [this](CCObject*) {
        auto doProfileInstead = [this]() {
            ProfilePage::create(m_data.accountID, false)->show();
        };
        if (auto scene = CCScene::get()) {
            if (auto commentsPopup = typeinfo_cast<CommentsPopup*>(scene->getChildByID("CommentsPopup"_spr))) {
                if (auto objectPopup = typeinfo_cast<ObjectPopup*>(scene->getChildByID("ObjectPopup"_spr))) {
                    auto workshop = objectPopup->getWorkshop();
                    if (workshop != nullptr) {
                        commentsPopup->onClose(nullptr);
                        objectPopup->onClose(nullptr);
                        workshop->onClickUser(m_data.accountID);
                    }
                } else {
                    doProfileInstead();
                }
            } else {
                doProfileInstead();
            }
        } else {
            doProfileInstead();
        }
    });
    authorBtn->setAnchorPoint({0, 0.5});
    authorBtn->setID("comment/author"_spr);
    
    // icon,playerColor,playerColor2,playerColorGlow,glow
    if (auto gm = GameManager::sharedState()) {
        SimplePlayer* pIcon;
        if (m_data.icon.size() != 5 || m_data.icon.empty()) {
            pIcon = SimplePlayer::create(2);
        } else {
            pIcon = SimplePlayer::create(m_data.icon[0]);
            pIcon->setColor(gm->colorForIdx(m_data.icon[1]));
            pIcon->setSecondColor(gm->colorForIdx(m_data.icon[2]));
            pIcon->setGlowOutline(gm->colorForIdx(m_data.icon[3]));
            pIcon->m_hasGlowOutline = m_data.icon[4] == 1;
            pIcon->updateColors();
        }
        pIcon->setScale(0.45f);
        bgSpr->addChildAtPosition(pIcon, Anchor::TopLeft, {11, -10});
    }
    //m_data.content = "AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA";
    auto commentText = TextArea::create(
        m_data.content, "chatFont.fnt",
        Utils::calculateScale(m_data.content, 16, 100, 0.6F, 0.4F),
        150.F, { 0.0f, 0.5f }, // 1.0
        Utils::calculateScale(m_data.content, 16, 100, 15.F, 7.F),
        true
    );
    commentText->setID("comment/area"_spr);
	commentText->setAnchorPoint({ 0.0f, 0.5f });
    ccColor3B textColor = {255, 255, 255};
    if (data.role == 2) {
        textColor = {0, 255, 0};
    }
    if (data.role == 3) {
        textColor = {221, 151, 254};
    }
    if (data.accountID == obj.authorAccId) {
        textColor = {255, 255, 0};
    }
    commentText->colorAllCharactersTo(textColor);

    auto invisLabel = CCLabelBMFont::create(m_data.content.c_str(), "chatFont.fnt");
    invisLabel->setScale(0.F);
    invisLabel->setVisible(false);
    invisLabel->setColor(textColor);
    invisLabel->setID("comment/content"_spr); // for emojis in comments or other mods

    auto timeAgo = CCLabelBMFont::create(m_data.timestamp.c_str(), "chatFont.fnt");
    timeAgo->setColor({0,0,0});
    timeAgo->setAnchorPoint({1, 0.5});
    timeAgo->setScale(0.35F);
    timeAgo->setOpacity(125);
    timeAgo->setID("comment/timestamp"_spr);

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
    votesLabel->setScale(0.3F);
    votesLabel->setID("comment/votes"_spr);

    bgSpr->addChildAtPosition(invisLabel, Anchor::Left, {4, 0});
    bgSpr->addChildAtPosition(commentText, Anchor::Left, {4, 0});
    menu->addChildAtPosition(authorBtn, Anchor::TopLeft, {22, -9});
    bgSpr->addChildAtPosition(timeAgo, Anchor::BottomRight, {-4, 5});

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
    if (!m_user.authenticated) return FLAlertLayer::create("Error", "You cannot vote on comments as you are <cy>not authenticated!</c>", "OK")->show();
    VotePopup::create("Vote", [this](bool like) {
        auto token = Mod::get()->getSettingValue<std::string>("token");
        m_listener.getFilter().cancel();
        m_listener.bind([this, token] (web::WebTask::Event* e) {
            if (web::WebResponse* value = e->getValue()) {
                auto jsonRes = value->json().unwrapOrDefault();
                if (!jsonRes.isObject()) return log::error("Response isn't object.");
                Utils::notifMessage(jsonRes, m_forceRefresh);
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
        myjson.set("like", (int)like);
        req.header("Content-Type", "application/json");
        req.userAgent(USER_AGENT);
        req.bodyJSON(myjson);
        m_listener.setFilter(req.post(fmt::format("{}/objects/{}/comments/{}/vote", HOST_URL, m_data.objectID, m_data.id)));
    })->show();
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
                        if (!jsonRes.isObject()) return log::error("Response isn't object.");
                        Utils::notifMessage(jsonRes, m_forceRefresh);
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
                req.userAgent(USER_AGENT);
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
                        if (!jsonRes.isObject()) return log::error("Response isn't object.");
                        Utils::notifMessage(jsonRes, m_forceRefresh);
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
                req.userAgent(USER_AGENT);
                m_listener.setFilter(req.post(fmt::format("{}/objects/{}/comments/{}/delete", HOST_URL, m_data.objectID, m_data.id)));
            }
        },
        true,
        true
    );
}

OWCommentCell* OWCommentCell::create(CommentData data, ObjectData obj, UserData user, std::function<void()> callback) {
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
