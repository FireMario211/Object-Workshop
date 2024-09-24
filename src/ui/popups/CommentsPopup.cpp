#include "CommentsPopup.hpp"
#include "CommentPopup.hpp"
#include "../../nodes/ScrollLayerExt.hpp"
#include "../../nodes/CommentCell.hpp"
#include "../../config.hpp"

bool CommentsPopup::setup(ObjectData object, UserData user) {
    m_object = object;
    m_user = user;
    this->setTitle("Comments (N/A)");
    pageLabel = CCLabelBMFont::create("", "goldFont.fnt");
    pageLabel->setScale(0.5F);
    m_mainLayer->addChildAtPosition(pageLabel, Anchor::Bottom, {0, 15});

    m_commentsBG = CCScale9Sprite::create("square02_small.png");
    m_commentsBG->setOpacity(50);
    m_commentsBG->setContentSize({205, 140});
    m_mainLayer->addChildAtPosition(m_commentsBG, Anchor::Center, {0, -5});

    auto leftArrowSpr = CCSprite::createWithSpriteFrameName("GJ_arrow_01_001.png");
    auto rightArrowSpr = CCSprite::createWithSpriteFrameName("GJ_arrow_01_001.png");
    rightArrowSpr->setFlipX(true);
    leftArrowSpr->setScale(0.75F);
    rightArrowSpr->setScale(0.75F);
    leftArrowBtn = CCMenuItemSpriteExtra::create(leftArrowSpr, this, menu_selector(CommentsPopup::onPrevPage));
    rightArrowBtn = CCMenuItemSpriteExtra::create(rightArrowSpr, this, menu_selector(CommentsPopup::onNextPage));
    m_buttonMenu->addChildAtPosition(leftArrowBtn, Anchor::Left, {25, -5});
    m_buttonMenu->addChildAtPosition(rightArrowBtn, Anchor::Right, {-25, -5});

    leftArrowBtn->setEnabled(false);
    rightArrowBtn->setEnabled(false);

    auto refreshSpr = CCSprite::createWithSpriteFrameName("GJ_updateBtn_001.png");
    refreshSpr->setScale(0.75F);
    auto refreshBtn = CCMenuItemSpriteExtra::create(refreshSpr, this, menu_selector(CommentsPopup::onLoadComments));
    m_buttonMenu->addChildAtPosition(refreshBtn, Anchor::BottomLeft, {3, 3});

    auto likesSpr = ButtonSprite::create(
        CCSprite::createWithSpriteFrameName("GJ_likesIcon_001.png"),
        32,
        0,
        32,
        1.0F,
        false,
        "GJ_button_01.png",
        true
    );
    likesSpr->setScale(0.6F);
    auto recentSpr = ButtonSprite::create(
        CCSprite::createWithSpriteFrameName("GJ_timeIcon_001.png"),
        32,
        0,
        32,
        1.0F,
        false,
        "GJ_button_01.png",
        true
    );
    recentSpr->setScale(0.6F);
    auto likesBtn = CCMenuItemSpriteExtra::create(likesSpr, this, menu_selector(CommentsPopup::onCategoryButton));
    auto recentBtn = CCMenuItemSpriteExtra::create(recentSpr, this, menu_selector(CommentsPopup::onCategoryButton));
    likesBtn->setTag(1);
    recentBtn->setTag(2);
    filterMenu = CCMenu::create();
    filterMenu->addChild(likesBtn);
    filterMenu->addChild(recentBtn);
    filterMenu->setLayout(ColumnLayout::create()->setAxisReverse(true));
    filterMenu->updateLayout();
    m_mainLayer->addChildAtPosition(filterMenu, Anchor::Left, {25, 39});

    auto commentSpr = CCSprite::createWithSpriteFrameName("GJ_chatBtn_001.png");
    commentSpr->setScale(0.65F);

    auto commentBtn = CCMenuItemSpriteExtra::create(commentSpr, this, menu_selector(CommentsPopup::onCommentBtn));
    m_buttonMenu->addChildAtPosition(commentBtn, Anchor::BottomRight, {-3, 3});

    onLoadComments(nullptr);
    updateCategoryBG();
    return true;
}

void CommentsPopup::onLoadComments(CCObject*) {
    m_commentsBG->removeChildByID("commentscroll"_spr);
    m_mainLayer->removeChildByID("loadingCircle");
    auto loadingCircle = LoadingCircle::create();
    loadingCircle->setPositionX(-(m_mainLayer->getContentWidth() / 2) + 15);
    loadingCircle->setPositionY(-(m_mainLayer->getContentHeight() / 2) + 40);
    loadingCircle->setScale(0.7F);
    loadingCircle->setParentLayer(m_mainLayer);
    loadingCircle->show();
    loadingCircle->setID("loadingCircle");
    m_listener.getFilter().cancel();
    m_listener.bind([this, loadingCircle] (web::WebTask::Event* e) {
        if (web::WebResponse* value = e->getValue()) {
            loadingCircle->fadeAndRemove();
            if (value->code() >= 500 && !value->ok()) {
                Notification::create("A server error occured. Check logs for info.", NotificationIcon::Error)->show();
                log::error("{}", value->string().unwrapOrDefault());
                return;
            }
            auto jsonRes = value->json().unwrapOrDefault();
            auto isError = jsonRes.try_get<std::string>("error");
            leftArrowBtn->setEnabled(false);
            rightArrowBtn->setEnabled(false);
            if (isError) return Notification::create(isError->c_str(), NotificationIcon::Error)->show();
            leftArrowBtn->setEnabled(true);
            rightArrowBtn->setEnabled(true);
            auto arrayRes = jsonRes.try_get<matjson::Array>("results");
            auto c_total = jsonRes.try_get<int>("total");
            auto c_page = jsonRes.try_get<int>("page");
            auto c_pageAmount = jsonRes.try_get<int>("pageAmount");
            matjson::Array array;
            if (arrayRes) {
                array = arrayRes.value();
            }
            if (c_total) {
                this->setTitle(fmt::format("Comments ({})", c_total.value()).c_str());
            }
            if (c_page) {
                m_object.commentPage = c_page.value();
            }
            if (c_pageAmount) {
                m_object.maxCommentPage = c_pageAmount.value();
            }
            auto nodeArray = CCArray::create();
            
            auto scrollLayer = ScrollLayerExt::create({ 0, 0, 205.0F, 138.F }, true);
            auto content = CCMenu::create();
            content->setZOrder(2);
            content->setContentWidth(205.0F);
            scrollLayer->m_contentLayer->setContentSize({
                content->getContentSize().width,
                (array.size() <= 3) ? 180.F : 45.F * array.size()
            });
            content->setContentHeight(scrollLayer->m_contentLayer->getContentHeight());
            content->registerWithTouchDispatcher();
            
            scrollLayer->m_contentLayer->addChild(content);
            scrollLayer->setTouchEnabled(true);
            for (auto item : array) {
                auto obj = item;
                auto o_id = obj.try_get<int>("id");
                auto o_object_id = obj.try_get<int>("object_id");
                auto o_acc_name = obj.try_get<std::string>("account_name");
                auto o_acc_id = obj.try_get<int>("account_id");
                auto o_likes = obj.try_get<int>("likes");
                auto o_content = obj.try_get<std::string>("content");
                auto o_timestamp = obj.try_get<std::string>("timestamp");
                auto o_icon = obj.try_get<matjson::Array>("icon");
                auto o_pinned = obj.try_get<bool>("pinned");
                auto o_role = obj.try_get<int>("role");
                if (
                    o_id && o_object_id &&
                    o_acc_name && o_acc_id &&
                    o_likes && o_content &&
                    o_timestamp && o_icon
                ) {
                    content->addChild(OWCommentCell::create({
                        o_id.value(),
                        o_object_id.value(),
                        o_acc_name.value(),
                        o_acc_id.value(),
                        o_timestamp.value(),
                        o_content.value(),
                        o_likes.value(),
                        o_pinned.value(),
                        o_icon.value(),
                        o_role.value()
                    }, m_object, m_user, [this]() {
                        onLoadComments(nullptr);
                    }));
                }
            }
            content->setLayout(
                ColumnLayout::create()
                    ->setAxisAlignment(AxisAlignment::End)
                    ->setCrossAxisOverflow(false)
                    ->setGap(5)
                    ->setGrowCrossAxis(true)
                    ->setAutoScale(false)
                    ->setAxisReverse(true)
            );
            if (array.size() <= 3) {
                scrollLayer->setTouchEnabled(false);
                scrollLayer->setMouseEnabled(false);
            }
            content->setAnchorPoint({0.5, 0});
            content->setPosition({102.5F, -2});
            scrollLayer->setID("commentscroll"_spr);
            m_commentsBG->addChildAtPosition(scrollLayer, Anchor::BottomLeft);
            scrollLayer->moveToTop();
            scrollLayer->fixTouchPrio();
            pageLabel->setString(fmt::format("Page {} of {}", m_object.commentPage, m_object.maxCommentPage).c_str());
        } else if (web::WebProgress* progress = e->getProgress()) {
            // The request is still in progress...
        } else if (e->isCancelled()) {
            log::error("Request was cancelled.");
        }
    });
    web::WebRequest req = web::WebRequest();
    req.userAgent(USER_AGENT);
    m_listener.setFilter(req.get(fmt::format("{}/objects/{}/comments?limit=10&page={}&filter={}", HOST_URL, m_object.id, m_object.commentPage, m_currentFilter)));
}

void CommentsPopup::onPrevPage(CCObject* sender) {
    if (m_object.commentPage > 1) {
        m_object.commentPage--;
        onLoadComments(sender);
    }
}
void CommentsPopup::onNextPage(CCObject* sender) {
    if ((m_object.commentPage + 1) <= m_object.maxCommentPage) {
        m_object.commentPage++;
        onLoadComments(sender);
    }
}

void CommentsPopup::updateCategoryBG() {
    if (auto likesBtn = typeinfo_cast<CCMenuItemSpriteExtra*>(filterMenu->getChildByTag(1))) {
        if (auto recentBtn = typeinfo_cast<CCMenuItemSpriteExtra*>(filterMenu->getChildByTag(2))) {
            static_cast<ButtonSprite*>(likesBtn->getChildren()->objectAtIndex(0))->updateBGImage((m_currentFilter != 1) ? "GJ_button_01.png" : "GJ_button_02.png");
            static_cast<ButtonSprite*>(recentBtn->getChildren()->objectAtIndex(0))->updateBGImage((m_currentFilter != 2) ? "GJ_button_01.png" : "GJ_button_02.png");
        }
    }
}

void CommentsPopup::onCategoryButton(CCObject* sender) {
    int id = static_cast<CCMenuItemSpriteExtra*>(sender)->getTag();
    m_currentFilter = id;
    updateCategoryBG();
    onLoadComments(sender);
}

void CommentsPopup::onCommentBtn(CCObject*) {
    if (!m_user.authenticated) return FLAlertLayer::create("Error", "You cannot comment on objects as you are <cy>not authenticated!</c>", "OK")->show();
    CommentPopup::create(m_object, [this]() {
        onLoadComments(nullptr);
    })->show();
}

void CommentsPopup::onClose(CCObject* sender) {
    m_listener.getFilter().cancel();
    Popup::onClose(sender);
}
