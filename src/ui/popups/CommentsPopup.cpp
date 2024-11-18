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
    leftArrowBtn = CCMenuItemExt::createSpriteExtra(leftArrowSpr, [this](CCObject* sender) {
        if (m_object.commentPage > 1) {
            m_object.commentPage--;
            onLoadComments(sender);
        }
    });
    rightArrowBtn = CCMenuItemExt::createSpriteExtra(rightArrowSpr, [this](CCObject* sender) {
        if ((m_object.commentPage + 1) <= m_object.maxCommentPage) {
            m_object.commentPage++;
            onLoadComments(sender);
        }
    });
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

    auto commentBtn = CCMenuItemExt::createSpriteExtraWithFrameName("GJ_chatBtn_001.png", 0.65F, [this](CCObject*) {
        if (!m_user.authenticated) return FLAlertLayer::create("Error", "You cannot comment on objects as you are <cy>not authenticated!</c>", "OK")->show();
        CommentPopup::create(m_object, [this]() {
            onLoadComments(nullptr);
        })->show();
    });
    m_buttonMenu->addChildAtPosition(commentBtn, Anchor::BottomRight, {-3, 3});

    onLoadComments(nullptr);
    this->setID("CommentsPopup"_spr);
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
            if (Utils::notifError(jsonRes)) {
                leftArrowBtn->setEnabled(false);
                rightArrowBtn->setEnabled(false);
                return;
            }
            leftArrowBtn->setEnabled(true);
            rightArrowBtn->setEnabled(true);
            auto arrayRes = jsonRes.get("results");
            auto c_total = jsonRes.get("total");
            auto c_page = jsonRes.get("page");
            auto c_pageAmount = jsonRes.get("pageAmount");
            std::vector<matjson::Value> array;
            if (arrayRes.isOk()) {
                array = arrayRes.unwrap().asArray().unwrap();
            }
            if (c_total.isOk()) {
                this->setTitle(fmt::format("Comments ({})", c_total.unwrap().asInt().unwrapOrDefault()).c_str());
            }
            if (c_page.isOk()) {
                m_object.commentPage = c_page.unwrap().asInt().unwrapOrDefault();
            }
            if (c_pageAmount.isOk()) {
                m_object.maxCommentPage = c_pageAmount.unwrap().asInt().unwrapOrDefault();
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
                auto commentRes = matjson::Serialize<CommentData>::fromJson(item);
                if (commentRes.isOk()) {
                    content->addChild(OWCommentCell::create(commentRes.unwrap(), m_object, m_user, [this]() {
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
    m_object.commentPage = 1;
    updateCategoryBG();
    onLoadComments(sender);
}

void CommentsPopup::onClose(CCObject* sender) {
    m_listener.getFilter().cancel();
    Popup::onClose(sender);
}
