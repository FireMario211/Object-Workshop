#include "../config.hpp"
#include "ObjectWorkshop.hpp"
#include "Geode/ui/ListView.hpp"
#include "popups/includes.h"
#include "../nodes/CategoryButton.hpp"
#include "../utils.hpp"
#include "../nodes/CommentCell.hpp"
//#include <dashauth.hpp>

int currentMenuIndexGD = 2;

bool ObjectWorkshop::setup(bool authenticated) {
    m_authenticated = authenticated;
    //m_authenticated = false;
    web::WebRequest req = web::WebRequest();
    m_tagsListener.getFilter().cancel();
    m_tagsListener.bind([this] (web::WebTask::Event* e) {
        if (web::WebResponse* value = e->getValue()) {
            if (value->code() >= 500 && !value->ok()) {
                Notification::create("A server error occured. Check logs for info.", NotificationIcon::Error)->show();
                log::error("{}", value->string().unwrapOrDefault());
                return;
            }
            auto jsonRes = value->json().unwrapOrDefault();
            if (jsonRes.is_object()) {
                auto isError = jsonRes.try_get<std::string>("error");
                if (isError) return Notification::create(isError->c_str(), NotificationIcon::Error)->show();
                log::error("{}", jsonRes.dump());
                return Notification::create("(Tags) Expected array, but got object.", NotificationIcon::Error)->show();
            }
            matjson::Array jsonArr = jsonRes.as_array();
            m_availableTags = Utils::arrayToUnorderedSet<std::string>(jsonArr);
            log::debug("Current available tags: {}", m_availableTags);
        } else if (web::WebProgress* progress = e->getProgress()) {
            // The request is still in progress...
        } else if (e->isCancelled()) {
            log::error("Request was cancelled.");
        }
    });
    m_tagsListener.setFilter(req.get(fmt::format("{}/objects/tags", HOST_URL)));
    objectInfoNode = CCNode::create();
    objectInfoNode->setAnchorPoint({0.5, 0.5});
    objectInfoNode->setContentSize({295, 225});

    auto backSpr = CCSprite::createWithSpriteFrameName("GJ_arrow_01_001.png");
    backSpr->setScale(0.5F);
    obj_backBtn = CCMenuItemSpriteExtra::create(
        backSpr,
        this,
        menu_selector(ObjectWorkshop::onBackBtn)
    );
    m_buttonMenu->addChildAtPosition(obj_backBtn, Anchor::Top, {-88, -18});
    obj_backBtn->setVisible(false);

    m_closeBtn->setZOrder(1);

    auto titleLabel = CCLabelBMFont::create("Custom Object Workshop", "goldFont.fnt");
    titleLabel->setScale(0.66F);
    m_mainLayer->addChildAtPosition(titleLabel, Anchor::Top, { 53, -17 });

    auto leftBar = CCScale9Sprite::create("square02_small.png");
    leftBar->setOpacity(60);
    leftBar->setContentSize({105, 280});
    leftBar->setAnchorPoint({0, 0.5});
    m_buttonMenu->addChildAtPosition(leftBar, Anchor::Left);

    auto leftTopBar = CCScale9Sprite::create("square02_001.png");
    leftTopBar->setOpacity(90);
    leftTopBar->setContentSize({155, 55});
    leftTopBar->setAnchorPoint({0.5, 1});
    leftTopBar->setScale(0.575F);

    auto profileSpr = CCSprite::createWithSpriteFrameName("GJ_profileButton_001.png"); // make clickable to go to profile
    profileSpr->setScale(0.75F);
    leftTopBar->addChildAtPosition(profileSpr, Anchor::Left, {27, 1});

    auto playerName = GameManager::sharedState()->m_playerName;
    auto profileLabel = CCLabelBMFont::create(playerName.c_str(), "goldFont.fnt");
    profileLabel->setAnchorPoint({0, 0.5});
    profileLabel->limitLabelWidth(90.F, 1.0F, 0.2F);
    leftTopBar->addChildAtPosition(profileLabel, Anchor::Center, {-25, 10});

    leftBar->addChildAtPosition(leftTopBar, Anchor::Top, {3, -7});

    createCategoryBtn("My Objects", 0);
    createCategoryBtn("Favorites", 1);

    auto vLine = CCSprite::createWithSpriteFrameName("edit_vLine_001.png");
    vLine->setRotation(90);
    leftBar->addChildAtPosition(vLine, Anchor::Center, {0, 40});

    /*createCategoryBtn("Trending", 2);
    createCategoryBtn("Top Downloads", 3);
    createCategoryBtn("Most Popular", 4);
    createCategoryBtn("Most Liked", 5);
    createCategoryBtn("Most Recent", 6);
    createCategoryBtn("Friends' Favorite", 7);*/

    m_categoryButtons = CCMenu::create();
    m_categoryButtons->setLayout(
        ColumnLayout::create()
            ->setAxisReverse(true)
            ->setAutoScale(false)
            ->setCrossAxisOverflow(false)
            ->setAxisAlignment(AxisAlignment::Even)
    );
    m_categoryButtons->setAnchorPoint({0.5, 1});
    m_categoryButtons->setContentSize({90, 122});

    createCategoryBtn("Top Downloads", 2); // 0
    createCategoryBtn("Most Popular", 3); // 1
    createCategoryBtn("Most Liked", 4); // 2
    createCategoryBtn("Trending", 5); // 3
    createCategoryBtn("Most Recent", 6); // 4

    m_searchInput = TextInput::create(110.0F, "Search...");
    m_searchInput->setMaxCharCount(64);
    m_searchInput->setScale(0.525F);
    m_searchInput->setTextAlign(TextInputAlign::Left);
    m_buttonMenu->addChildAtPosition(m_searchInput, Anchor::BottomLeft, { 38, 18 });
    m_buttonMenu->addChildAtPosition(m_categoryButtons, Anchor::Left, {55, 32});

    auto searchSpr = CCSprite::createWithSpriteFrameName("gj_findBtn_001.png");
    searchSpr->setScale(0.525F);
    auto searchBtn = CCMenuItemSpriteExtra::create(
        searchSpr,
        this,
        menu_selector(ObjectWorkshop::onSearchBtn)
    );
    m_buttonMenu->addChildAtPosition(searchBtn, Anchor::BottomLeft, { 93, 18 });
    if (auto item = typeinfo_cast<CCMenuItemSpriteExtra*>(m_categoryButtons->getChildByID(fmt::format("category-{}"_spr, currentMenuIndexGD)))) {
        static_cast<CategoryButton*>(item->getChildren()->objectAtIndex(0))->setIndicatorState(true);
    }
    if (auto item = typeinfo_cast<CCMenuItemSpriteExtra*>(m_buttonMenu->getChildByID(fmt::format("category-{}"_spr, currentMenuIndexGD)))) {
        static_cast<CategoryButton*>(item->getChildren()->objectAtIndex(0))->setIndicatorState(true);
    }

    auto filterSpr = ButtonSprite::create(
        CCSprite::createWithSpriteFrameName("GJ_filterIcon_001.png"),
        30,
        0,
        .0F,
        1.0F,
        false,
        "GJ_button_04.png",
        false
    );
    filterSpr->setScale(0.4F);
    auto filterBtn = CCMenuItemSpriteExtra::create(
        filterSpr,
        this,
        menu_selector(ObjectWorkshop::onFilterBtn)
    );
    m_buttonMenu->addChildAtPosition(filterBtn, Anchor::BottomLeft, { 76, 18 });

    auto refreshSpr = CCSprite::createWithSpriteFrameName("GJ_updateBtn_001.png");
    refreshSpr->setScale(0.75F);
    auto refreshBtn = CCMenuItemSpriteExtra::create(refreshSpr, this, menu_selector(ObjectWorkshop::onReloadBtn));
    m_buttonMenu->addChildAtPosition(refreshBtn, Anchor::BottomRight, {-8, 8});

/*
    auto unlistedSpr = CCSprite::createWithSpriteFrameName("accountBtn_myLevels_001.png");
    unlistedSpr->setScale(0.425F);
    auto unlistedBtn = CCMenuItemSpriteExtra::create(
        unlistedSpr,
        this,
        menu_selector(ObjectWorkshop::onPendingBtn)
    );
    unlistedBtn->setVisible(false);
    m_buttonMenu->addChildAtPosition(unlistedBtn, Anchor::BottomLeft, { 17, 41 });;
*/

    rightBg = CCScale9Sprite::create("square02_small.png");
    rightBg->setOpacity(60);
    rightBg->setContentSize({295, 225});
    m_mainLayer->addChildAtPosition(rightBg, Anchor::Center, {53, -4});

    m_mainLayer->addChildAtPosition(objectInfoNode, Anchor::Center, {53, -4});

    m_scrollLayer = ScrollLayerExt::create({ 0, 0, 295.0F, 225.0F }, true);
    //m_scrollLayer->setPosition(_w - 273, 45);
    m_content = CCMenu::create();
    m_content->setZOrder(2);
    m_content->setPositionX(20);
    m_content->registerWithTouchDispatcher();

    m_scrollLayer->m_contentLayer->addChild(m_content);

    m_scrollLayer->setTouchEnabled(true);
    m_scrollLayer->setMouseEnabled(true);

    rightBg->addChild(m_scrollLayer);

    if (m_authenticated) {
        auto token = Mod::get()->getSettingValue<std::string>("token");
        m_token = token;
        m_listener0.getFilter().cancel();
        m_listener0.bind([this, leftTopBar] (web::WebTask::Event* e) {
            if (web::WebResponse* value = e->getValue()) {
                auto jsonRes = value->json().unwrapOrDefault();
                if (!jsonRes.is_object()) return log::error("Response isn't object.");
                auto isError = jsonRes.try_get<std::string>("error");
                if (isError) return Notification::create(isError->c_str(), NotificationIcon::Error)->show();
                // i know its a lotta more lines but gotta check!
                auto r_account_id = jsonRes.try_get<int>("account_id");
                auto r_name = jsonRes.try_get<std::string>("name");
                auto r_downloaded = jsonRes.try_get<matjson::Array>("downloaded");
                auto r_favorites = jsonRes.try_get<matjson::Array>("favorites");
                auto r_uploads = jsonRes.try_get<int>("uploads");
                auto r_role = jsonRes.try_get<int>("role");
                if (r_account_id && r_name && r_downloaded && r_favorites && r_uploads && r_role) {
                    m_user = {
                        r_account_id.value(),
                        r_name->c_str(),
                        r_downloaded.value(),
                        r_favorites.value(),
                        r_uploads.value(),
                        r_role.value()
                    };
                    log::debug("Set user information.");
                } else {
                    log::error("Something went wrong when getting keys from the users object. {}", jsonRes.dump());
                    Notification::create("Couldn't parse user object.", NotificationIcon::Warning)->show();
                }
                auto uploadsLabel = CCLabelBMFont::create(fmt::format("{} Upload{}", m_user.uploads, (m_user.uploads == 1) ? "" : "s").c_str(), "bigFont.fnt");
                uploadsLabel->limitLabelWidth(90.F, 1.0F, 0.2F);
                uploadsLabel->setAnchorPoint({0, 0.5});
                for (int i = 0; i < std::to_string(m_user.uploads).length(); i++) {
                    // why doesnt CCFontSprite EXIST!?
                    auto fontSpr = static_cast<CCSprite*>(uploadsLabel->getChildren()->objectAtIndex(i));
                    fontSpr->setColor({0,255,0});
                }
                leftTopBar->addChildAtPosition(uploadsLabel, Anchor::Center, {-22, -11});
                if (m_user.role >= 2) {
                    createCategoryBtn("Pending", 7);
                    if (m_user.role == 3) {
                        createCategoryBtn("Reports", 8);
                    }
                }
            } else if (web::WebProgress* progress = e->getProgress()) {
                // The request is still in progress...
            } else if (e->isCancelled()) {
                log::error("Request was cancelled.");
            }
        });
        web::WebRequest req = web::WebRequest();
        auto myjson = matjson::Value();
        myjson.set("token", m_token);
        req.header("Content-Type", "application/json");
        req.bodyJSON(myjson);
        m_listener0.setFilter(req.post(fmt::format("{}/user/@me", HOST_URL)));
    }
    auto pagesMenu = CCMenu::create();
    m_buttonMenu->addChildAtPosition(pagesMenu, Anchor::BottomLeft, {55, 41});

    auto leftArrowSpr = CCSprite::createWithSpriteFrameName("GJ_arrow_01_001.png");
    auto rightArrowSpr = CCSprite::createWithSpriteFrameName("GJ_arrow_01_001.png");
    rightArrowSpr->setFlipX(true);
    leftArrowSpr->setScale(0.5F);
    rightArrowSpr->setScale(0.5F);
    auto leftArrowBtn = CCMenuItemSpriteExtra::create(leftArrowSpr, this, menu_selector(ObjectWorkshop::onLeftPage));
    auto rightArrowBtn = CCMenuItemSpriteExtra::create(rightArrowSpr, this, menu_selector(ObjectWorkshop::onRightPage));
    leftArrowBtn->setPositionX(-30);
    rightArrowBtn->setPositionX(30);
    pagesMenu->addChild(leftArrowBtn);
    pagesMenu->addChild(rightArrowBtn);

    /*pageLabel = CCLabelBMFont::create(std::to_string(m_currentPage).c_str(), "bigFont.fnt");
    pageLabel->setScale(0.5F);
    pagesMenu->addChild(pageLabel);*/

    m_pageInput = TextInput::create(70.0F, "Page...");
    m_pageInput->setString("1");
    m_pageInput->setMaxCharCount(8);
    m_pageInput->setScale(0.525F);
    m_pageInput->setCommonFilter(CommonFilter::Uint);
    m_pageInput->setDelegate(this);
    pagesMenu->addChild(m_pageInput);

    RegenCategory();

    this->setID("objectworkshop"_spr);
    log::debug("Finished with setup!");
    //cocos::handleTouchPriority(m_scrollLayer);
    return true;
}

void ObjectWorkshop::onSideButton(CCObject* pSender) {
    isSearching = false;
    m_currentPage = 1;
    m_pageInput->setString("1");
    onBackBtn(pSender);
    auto item = static_cast<CCMenuItemSpriteExtra*>(pSender);
    auto idStr = item->getID();
    int id = std::stoi(idStr.substr(idStr.length() - 1));
    if (id != currentMenuIndexGD) {
        if (auto oldItem = typeinfo_cast<CCMenuItemSpriteExtra*>(m_buttonMenu->getChildByID(fmt::format("category-{}"_spr, currentMenuIndexGD)))) {
            static_cast<CategoryButton*>(oldItem->getChildren()->objectAtIndex(0))->setIndicatorState(false);
        }
        if (auto oldItem = typeinfo_cast<CCMenuItemSpriteExtra*>(m_categoryButtons->getChildByID(fmt::format("category-{}"_spr, currentMenuIndexGD)))) {
            static_cast<CategoryButton*>(oldItem->getChildren()->objectAtIndex(0))->setIndicatorState(false);
        }
        static_cast<CategoryButton*>(item->getChildren()->objectAtIndex(0))->setIndicatorState(true);
        currentMenuIndexGD = id;
        RegenCategory();
    }
}

void ObjectWorkshop::onLeftPage(CCObject*) {
    if (m_currentPage > 1) {
        m_currentPage--;
        m_pageInput->setString(std::to_string(m_currentPage).c_str());
        RegenCategory();
    }
}
void ObjectWorkshop::onRightPage(CCObject*) {
    if ((m_currentPage + 1) <= m_maxPage) {
        m_currentPage++;
        m_pageInput->setString(std::to_string(m_currentPage).c_str());
        RegenCategory();
    }
}

void ObjectWorkshop::textInputOpened(CCTextInputNode* input) {
    input->setString("");
}

void ObjectWorkshop::textInputClosed(CCTextInputNode* input) {
    if (input->getString().empty()) return m_pageInput->setString(std::to_string(m_currentPage).c_str());
    int page = std::stoi(input->getString());
    if (page == m_currentPage) return m_pageInput->setString(std::to_string(m_currentPage).c_str());
    if (page < 1) page = 1;
    if (page <= m_maxPage) {
        m_currentPage = page;
        m_pageInput->setString(std::to_string(m_currentPage).c_str());
        RegenCategory();
    }
}

void ObjectWorkshop::RegenCategory() {
    int myItems = 0;
    int items = 0;
    log::debug("RegenCategory ({},{})", currentMenuIndexGD, m_currentPage);

    loadingCircle = LoadingCircle::create();
    m_buttonMenu->removeChildByID("retrybtn"_spr);
    m_content->removeAllChildrenWithCleanup(true);
    myUploadsBar = CCNode::create();
    auto myUploadsLabel = CCLabelBMFont::create("My Uploads", "bigFont.fnt");
    //myUploadsLabel->setScale(0.475F);
    myUploadsLabel->limitLabelWidth(90.0F, 0.8F, 0.2F);
    auto mUbarLeft = BreakLine::create(90.F);
    auto mUbarRight = BreakLine::create(90.F);
    mUbarRight->setAnchorPoint({1, 0});
    myUploadsBar->addChildAtPosition(mUbarLeft, Anchor::Left);
    myUploadsBar->addChildAtPosition(myUploadsLabel, Anchor::Center);
    myUploadsBar->addChildAtPosition(mUbarRight, Anchor::Right);

    myUploadsBar->setContentSize({280, 25});
    myUploadsBar->setAnchorPoint({0.5, 0.5});

    myUploadsBar->setLayout(
        RowLayout::create()
            ->setAxisAlignment(AxisAlignment::Even)
            ->setAutoScale(false)
            ->setCrossAxisOverflow(false)
            ->setGrowCrossAxis(true)
    );

    myUploadsBar->updateLayout();
    m_content->addChild(myUploadsBar);

    // unFORTUNATELY i do have to create a ccmenu
    myUploadsMenu = CCMenu::create();
    //myUploadsMenu->setPosition({127,-5});

    myUploadsMenu->setPosition({125,-5});
    myUploadsMenu->setContentSize(m_scrollLayer->getContentSize());
    myUploadsMenu->setAnchorPoint({0.5, 0});
    myUploadsMenu->setLayout(
        RowLayout::create()
            ->setAxisAlignment(AxisAlignment::Center)
            ->setAutoScale(false)
            ->setCrossAxisOverflow(false)
            //->setGap(25)
            ->setGap(8)
            ->setGrowCrossAxis(true)
    );
    auto uploadSpr = CCSprite::create("upload.png"_spr);
    uploadSpr->setScale(0.625F);
    auto uploadBtn = CCMenuItemSpriteExtra::create(uploadSpr, this, menu_selector(ObjectWorkshop::onUploadBtn));
    myUploadsMenu->addChild(uploadBtn);
    m_content->addChild(myUploadsMenu);
    myUploadsMenu->updateLayout();

    categoryBar = CCNode::create();
    /*categoryBar->setLayout(
        RowLayout::create()
            ->setAxisAlignment(AxisAlignment::Even)
            ->setAutoScale(false)
            ->setCrossAxisOverflow(false)
            ->setGrowCrossAxis(true)
    );*/
    auto categoryLabel = CCLabelBMFont::create(Utils::menuIndexToString(currentMenuIndexGD).c_str(), "bigFont.fnt");

    // 0.4
    categoryLabel->limitLabelWidth(120.F, 0.7F, 0.25F);
    /*auto barLeft = BreakLine::create((categoryLabel->getScale() * 100) + 5);
    auto barRight = BreakLine::create((categoryLabel->getScale() * 100) + 5);*/
    int barSize = 40;
    switch (currentMenuIndexGD) {
        case 0:
        case 3:
        case 4:
        case 5:
        case 2:
        default:
            barSize = 70;
            break;
        case 1:
        case 6:
        case 7:
        case 8:
            barSize = 80;
            break;
    }
    // because idk the math formula
    auto barLeft = BreakLine::create(barSize);
    auto barRight = BreakLine::create(barSize);
    barRight->setAnchorPoint({1, 0});
    categoryBar->addChildAtPosition(barLeft, Anchor::Left);
    categoryBar->addChildAtPosition(categoryLabel, Anchor::Center);
    categoryBar->addChildAtPosition(barRight, Anchor::Right);

    categoryBar->setContentSize({280, 25});
    categoryBar->setAnchorPoint({0.5, 0.5});
    categoryBar->updateLayout();

    categoryBar->updateLayout();
    m_content->addChild(categoryBar);

    categoryItems = CCMenu::create();
    categoryItems->setPosition({0,0});

    categoryItems->setContentSize(m_scrollLayer->getContentSize());
    categoryItems->setAnchorPoint({0.5, 0});
    categoryItems->setLayout(
        RowLayout::create()
            ->setAxisAlignment(AxisAlignment::Start)
            ->setCrossAxisAlignment(AxisAlignment::Center)
            ->setAutoScale(false)
            ->setCrossAxisOverflow(false)
            ->setGap(8)
            ->setGrowCrossAxis(true)
    );

    categoryItems->updateLayout();

    m_content->addChild(categoryItems);

    loadingCircle->setPosition({-158, -158});
    loadingCircle->setParentLayer(m_content);
    loadingCircle->show();

    myUploadsBar->setPosition({127, 138});
    categoryBar->setPosition({127, 74});
    categoryItems->setPosition({138, -105});

    m_scrollLayer->m_contentLayer->setContentSize({
        m_content->getContentSize().width,
        m_content->getContentSize().height - 90
    });
    m_content->setPositionY(80);
    m_scrollLayer->setTouchEnabled(false);
    
    if (currentMenuIndexGD != 0) {
        myUploadsBar->setVisible(false);
        myUploadsMenu->setVisible(false);
        m_content->setPositionY(140);
    }
    m_scrollLayer->moveToTop();
    cocos::handleTouchPriority(m_buttonMenu);
    load();
}

void ObjectWorkshop::onRetryBtn(CCObject*) {
    ObjectWorkshop::RegenCategory();
}

void ObjectWorkshop::load() {
    log::debug("Loading objects...");
    web::WebRequest request = web::WebRequest();
    m_listener1.getFilter().cancel();
    m_listener2.getFilter().cancel();
    categoryItems->setVisible(false);
    myUploadsMenu->setVisible(false);
    m_listener1.bind([this] (web::WebTask::Event* e) {
        if (web::WebResponse* value = e->getValue()) {
            log::debug("Finished request for listener 1.");
            if (!value->ok()) {
                auto errorText = value->string()->c_str();
                log::error("Failed to retrieve data from server: {}", errorText);
                auto errorLabel = CCLabelBMFont::create("Error fetching objects", "bigFont.fnt");
                errorLabel->setScale(0.6F);
                auto retrySpr = ButtonSprite::create("Retry", "bigFont.fnt", "GJ_button_04.png", .8f);
                retrySpr->setScale(0.6F);
                auto retryBtn = CCMenuItemSpriteExtra::create(retrySpr, this, menu_selector(ObjectWorkshop::onRetryBtn));
                retryBtn->setID("retrybtn"_spr);
                auto detailsLabel = CCLabelBMFont::create(value->string()->c_str(), "chatFont.fnt");
                detailsLabel->setAnchorPoint({0.5, 1.0});
                detailsLabel->limitLabelWidth(180.F, 1.0F, 0.25F);
                m_content->addChildAtPosition(errorLabel, Anchor::Center, {132, 80});
                //m_buttonMenu->addChildAtPosition(retryBtn, Anchor::Center, {132, 50});
                m_buttonMenu->addChildAtPosition(retryBtn, Anchor::Center, {55, 10});
                m_content->addChildAtPosition(detailsLabel, Anchor::Center, {132, 30});
                if (loadingCircle != nullptr) loadingCircle->fadeAndRemove();
                m_scrollLayer->moveToTop();
                return;
            }
            auto jsonRes = value->json().unwrapOrDefault();
            if (!jsonRes.is_object()) return log::error("Response isn't object.");
            auto isError = jsonRes.try_get<std::string>("error");
            if (isError) return Notification::create(isError->c_str(), NotificationIcon::Error)->show();
            if (!value->ok()) return Notification::create("An unknown error occured.", NotificationIcon::Error)->show();
            auto arrayRes = jsonRes.try_get<matjson::Array>("results");
            matjson::Array array;
            if (arrayRes) {
                array = arrayRes.value();
            }
            for (auto item : array) {
                auto obj = item;
                // soooo bad
                auto o_id = obj.try_get<int>("id");
                auto o_name = obj.try_get<std::string>("name");
                auto o_desc = obj.try_get<std::string>("description");
                auto o_acc_name = obj.try_get<std::string>("account_name");
                auto o_rating = obj.try_get<double>("rating");
                auto o_acc_id = obj.try_get<int>("account_id");
                auto o_downloads = obj.try_get<int>("downloads");
                auto o_favorites = obj.try_get<int>("favorites");
                auto o_rating_count = obj.try_get<int>("rating_count");
                auto o_data = obj.try_get<std::string>("data");
                auto o_tags = obj.try_get<matjson::Array>("tags");
                auto o_status = obj.try_get<int>("status");
                auto o_created = obj.try_get<std::string>("created");
                auto o_updated = obj.try_get<std::string>("updated");
                auto o_version = obj.try_get<int>("version");
                if (
                    o_id &&
                    o_name && o_desc &&
                    o_acc_name && o_acc_id &&
                    o_rating && o_downloads && o_favorites && o_rating_count && 
                    o_data && o_tags && o_status && o_created && o_updated && o_version
                ) {
                    auto cell = CCMenuItemSpriteExtra::create(ObjectItem::create({
                        o_id.value(),
                        o_name.value(),
                        o_desc.value(),
                        o_acc_name.value(),
                        o_rating.value(),
                        o_acc_id.value(),
                        o_downloads.value(),
                        o_favorites.value(),
                        o_rating_count.value(),
                        o_data.value(),
                        Utils::arrayIncludes(m_user.favorites, o_id.value()),
                        Utils::arrayToUnorderedSet<std::string>(o_tags.value()),
                        o_status.value() == 0,
                        o_created.value(),
                        o_updated.value(),
                        o_version.value()
                    }), this, menu_selector(ObjectWorkshop::onClickObject));
                    categoryItems->addChild(cell);
                } else {
                    log::error("One of the cells could not be properly parsed! {}", obj.dump());
                }
            }
            m_buttonMenu->removeChildByID("bottompage"_spr);
            auto o_page = jsonRes.try_get<int>("page");
            auto o_pageAmount = jsonRes.try_get<int>("pageAmount");
            auto o_total = jsonRes.try_get<int>("total");
            if (o_page && o_pageAmount && o_total) {
                bottomPageLabel = CCLabelBMFont::create(fmt::format("Page {} of {} ({} Results found)", o_page.value(), o_pageAmount.value(), o_total.value()).c_str(), "goldFont.fnt");
                m_maxPage = o_pageAmount.value();
            }
            bottomPageLabel->setScale(0.4F);
            bottomPageLabel->setID("bottompage"_spr);
            m_buttonMenu->addChildAtPosition(bottomPageLabel, Anchor::Bottom, {55, 17});
            m_scrollLayer->setTouchEnabled(true);
            m_amountItems = array.size();
            if (!m_authenticated || currentMenuIndexGD != 0) {
                if (m_amountItems > 6) {
                    categoryItems->setContentSize({
                        categoryItems->getContentWidth(),
                        310.F
                    });
                    myUploadsBar->setPosition({127, 138});
                    categoryBar->setPosition({127, 74});
                    categoryItems->setPosition({138, -250});

                    m_scrollLayer->m_contentLayer->setContentSize({
                        m_content->getContentSize().width,
                        m_content->getContentSize().height + 30
                    });
                    m_content->setPositionY(260);
                } else {
                    categoryItems->setContentSize({
                        categoryItems->getContentWidth(),
                        225.F
                    });
                    myUploadsBar->setPosition({127, 138});
                    categoryBar->setPosition({127, 74});
                    categoryItems->setPosition({138, -160});

                    m_scrollLayer->m_contentLayer->setContentSize({
                        m_content->getContentSize().width,
                        m_content->getContentSize().height - 70
                    });
                    m_content->setPositionY(160);
                }

                myUploadsMenu->removeAllChildrenWithCleanup(true);
                myUploadsBar->setPosition({127, 285});


                myUploadsMenu->updateLayout();
                myUploadsBar->updateLayout();
                categoryBar->updateLayout();
                categoryItems->updateLayout();
                if (loadingCircle != nullptr) loadingCircle->fadeAndRemove();
                cocos::handleTouchPriority(m_content);
                m_scrollLayer->fixTouchPrio();
                categoryItems->setVisible(true);
                myUploadsMenu->setVisible(true);
            }
            m_scrollLayer->moveToTop();
            if (m_authenticated && currentMenuIndexGD == 0) {
                categoryItems->setContentSize({
                    categoryItems->getContentWidth(),
                    225.F
                });
                web::WebRequest request2 = web::WebRequest();
                auto myjson = matjson::Value();
                myjson.set("token", m_token);
                request2.header("Content-Type", "application/json");
                request2.bodyJSON(myjson);
                m_listener2.setFilter(request2.get(fmt::format("{}/user/@me/objects?page=0&limit=true", HOST_URL)));
            }
        } else if (web::WebProgress* progress = e->getProgress()) {
            // The request is still in progress...
        } else if (e->isCancelled()) {
            log::error("Request was cancelled.");
        }
    });
    m_listener2.bind([this] (web::WebTask::Event* e) {
        if (web::WebResponse* value = e->getValue()) {
            log::debug("Finished request for listener 2.");
            auto jsonRes = value->json().unwrapOrDefault();
            if (!jsonRes.is_object()) return log::error("Response isn't object.");
            auto isError = jsonRes.try_get<std::string>("error");
            if (isError) return Notification::create(isError->c_str(), NotificationIcon::Error)->show();
            if (!value->ok()) return Notification::create("An unknown error occured.", NotificationIcon::Error)->show();
            auto arrayRes = jsonRes.try_get<matjson::Array>("results");
            matjson::Array array;
            if (arrayRes) {
                array = arrayRes.value();
            }
            if (array.size() > 0) {
                myUploadsMenu->removeAllChildrenWithCleanup(true);
            }
            CCSize defaultContentSize;
            for (auto item : array) {
                auto obj = item;
                // soooo bad
                auto o_id = obj.try_get<int>("id");
                auto o_name = obj.try_get<std::string>("name");
                auto o_desc = obj.try_get<std::string>("description");
                auto o_acc_name = obj.try_get<std::string>("account_name");
                auto o_rating = obj.try_get<double>("rating");
                auto o_acc_id = obj.try_get<int>("account_id");
                auto o_downloads = obj.try_get<int>("downloads");
                auto o_favorites = obj.try_get<int>("favorites");
                auto o_rating_count = obj.try_get<int>("rating_count");
                auto o_data = obj.try_get<std::string>("data");
                auto o_tags = obj.try_get<matjson::Array>("tags");
                auto o_status = obj.try_get<int>("status");
                auto o_created = obj.try_get<std::string>("created");
                auto o_updated = obj.try_get<std::string>("updated");
                auto o_version = obj.try_get<int>("version");
                if (
                    o_id &&
                    o_name && o_desc &&
                    o_acc_name && o_acc_id &&
                    o_rating && o_downloads && o_favorites && o_rating_count && 
                    o_data && o_tags && o_status && o_created && o_updated && o_version
                ) {
                    auto cell = CCMenuItemSpriteExtra::create(ObjectItem::create({
                        o_id.value(),
                        o_name.value(),
                        o_desc.value(),
                        o_acc_name.value(),
                        o_rating.value(),
                        o_acc_id.value(),
                        o_downloads.value(),
                        o_favorites.value(),
                        o_rating_count.value(),
                        o_data.value(),
                        Utils::arrayIncludes(m_user.favorites, o_id.value()),
                        Utils::arrayToUnorderedSet<std::string>(o_tags.value()),
                        o_status.value() == 0,
                        o_created.value(),
                        o_updated.value(),
                        o_version.value()
                    }), this, menu_selector(ObjectWorkshop::onClickObject));
                    defaultContentSize = cell->getContentSize();
                    myUploadsMenu->addChild(cell);
                } else {
                    log::error("One of the cells could not be properly parsed! {}", obj.dump());
                }
            }
            if (array.size() > 0) {
                myUploadsMenu->setPosition({127,-36});
                auto uploadSpr = CCSprite::create("upload.png"_spr);
                uploadSpr->setScale(0.625F);
                auto uploadBtn = CCMenuItemSpriteExtra::create(uploadSpr, this, menu_selector(ObjectWorkshop::onUploadBtn));
                uploadBtn->setContentSize({defaultContentSize.width, uploadBtn->getContentSize().height });
                auto withinUploadBtn = static_cast<CCSprite*>(uploadBtn->getChildren()->objectAtIndex(0)); // WHY
                withinUploadBtn->setPositionX(43.188F);
                myUploadsMenu->addChild(uploadBtn);
                myUploadsMenu->setLayout(
                    RowLayout::create()
                        ->setAxisAlignment(AxisAlignment::Center)
                        ->setAutoScale(false)
                        ->setCrossAxisOverflow(false)
                        //->setGap(25)
                        ->setGap(8)
                        ->setGrowCrossAxis(true)
                );
            } else {
                myUploadsMenu->setPosition({127,-5});
                myUploadsMenu->setLayout(
                    RowLayout::create()
                        ->setAxisAlignment(AxisAlignment::Center)
                        ->setAutoScale(false)
                        ->setCrossAxisOverflow(false)
                        //->setGap(25)
                        ->setGap(8)
                        ->setGrowCrossAxis(true)
                );
            }
            m_scrollLayer->setTouchEnabled(true);
            if (m_amountItems > 3) {
                if (array.size() > 0) {
                    myUploadsBar->setPosition({127, 138});
                    categoryBar->setPosition({127, 14});
                    categoryItems->setPosition({138, -220});

                    m_scrollLayer->m_contentLayer->setContentSize({
                        m_content->getContentSize().width,
                        m_content->getContentSize().height + 45
                    });
                    m_content->setPositionY(215);
                } else {
                    myUploadsBar->setPosition({127, 138});
                    categoryBar->setPosition({127, 74});
                    categoryItems->setPosition({138, -160});

                    m_scrollLayer->m_contentLayer->setContentSize({
                        m_content->getContentSize().width,
                        m_content->getContentSize().height - 10
                    });
                    m_content->setPositionY(154);
                }
            } else {
                if (array.size() > 0) {
                    myUploadsBar->setPosition({127, 138});
                    categoryBar->setPosition({127, 14});
                    categoryItems->setPosition({138, -165});

                    m_scrollLayer->m_contentLayer->setContentSize({
                        m_content->getContentSize().width,
                        m_content->getContentSize().height - 45
                    });
                    m_content->setPositionY(125);
                } else {
                    myUploadsBar->setPosition({127, 138});
                    categoryBar->setPosition({127, 74});
                    categoryItems->setPosition({138, -105});

                    m_scrollLayer->m_contentLayer->setContentSize({
                        m_content->getContentSize().width,
                        m_content->getContentSize().height - 90
                    });
                    m_content->setPositionY(80);
                    m_scrollLayer->setTouchEnabled(false);
                }
            }
            categoryItems->setVisible(true);
            myUploadsMenu->setVisible(true);
            myUploadsMenu->updateLayout();
            myUploadsBar->updateLayout();
            categoryBar->updateLayout();
            categoryItems->updateLayout();
            cocos::handleTouchPriority(m_content);
            m_scrollLayer->fixTouchPrio();
            if (loadingCircle != nullptr) loadingCircle->fadeAndRemove();
            m_scrollLayer->moveToTop();
        } else if (web::WebProgress* progress = e->getProgress()) {
            // The request is still in progress...
        } else if (e->isCancelled()) {
            log::error("Request was cancelled.");
        }
    });
    if (isSearching && !m_searchInput->getString().empty()) {
        std::string query = Utils::url_encode(m_searchInput->getString());
        if (m_filterTags.empty()) {
            m_listener1.setFilter(request.post(fmt::format("{}/objects/search?query={}&limit={}&page={}", HOST_URL, query, RESULT_LIMIT, m_currentPage)));
        } else {
            m_listener1.setFilter(
                request.post(
                    fmt::format(
                        "{}/objects/search?query={}&page={}&limit={}&tags={}",
                        HOST_URL,
                        query,
                        m_currentPage,
                        RESULT_LIMIT,
                        fmt::join(m_filterTags, ",")
                    )
                )
            );
        }
        return;
    }
    if (currentMenuIndexGD < 2 || currentMenuIndexGD > 6) {
        if (m_authenticated) {
            auto myjson = matjson::Value();
            myjson.set("token", m_token);
            request.header("Content-Type", "application/json");
            request.bodyJSON(myjson);
            if (currentMenuIndexGD == 0) { // my uploads
                m_listener1.setFilter(request.get(fmt::format("{}/user/@me/objects?page=0&limit=false", HOST_URL, m_currentPage)));
            } else if (currentMenuIndexGD == 1) { // favorited
                m_listener1.setFilter(request.post(fmt::format("{}/user/@me/favorites?page={}&limit={}", HOST_URL, m_currentPage, RESULT_LIMIT)));
            } else if (currentMenuIndexGD == 7) { // pending 
                m_listener1.setFilter(request.post(fmt::format("{}/objects/pending?page={}", HOST_URL, m_currentPage)));
            } else if (currentMenuIndexGD == 8) { // reports
                m_listener1.setFilter(request.post(fmt::format("{}/objects/reports?page={}", HOST_URL, m_currentPage)));
            }
        } else {
            FLAlertLayer::create("Error", "You aren't <cy>authenticated!</c>", "OK")->show();
            m_listener1.setFilter(request.get(fmt::format("{}/objects?page={}&category={}&limit={}", HOST_URL, m_currentPage, 0, RESULT_LIMIT)));
        }
    } else {
        if (m_filterTags.empty()) {
            m_listener1.setFilter(request.get(fmt::format("{}/objects?page={}&category={}&limit={}", HOST_URL, m_currentPage, currentMenuIndexGD - 2, RESULT_LIMIT)));
        } else {
            m_listener1.setFilter(
                request.get(
                    fmt::format(
                        "{}/objects?page={}&category={}&tags={}&limit={}",
                        HOST_URL,
                        m_currentPage,
                        currentMenuIndexGD - 2,
                        fmt::join(m_filterTags, ","),
                        RESULT_LIMIT
                    )
                )
            );
        }
    }
}

void ObjectWorkshop::createCategoryBtn(const char* string, int menuIndex) {
    auto label = CCLabelBMFont::create(string, "bigFont.fnt");
    label->limitLabelWidth(160.0F, 0.75F, 0.2F);
    
    /*auto bgNode = cocos2d::extension::CCScale9Sprite::create("square02b_001.png");
    bgNode->setOpacity(0);
    bgNode->setContentSize({ 90.0f, 20.0f });*/ 
    auto bgNode = CategoryButton::create(string);
    auto btn = CCMenuItemSpriteExtra::create(
        bgNode,
        this,
        menu_selector(ObjectWorkshop::onSideButton)
    );

    btn->setID(fmt::format("category-{}"_spr, menuIndex));
    
    //m_buttonMenu->addChildAtPosition(CategoryButton::create("My Objects", nullptr), Anchor::Top, {3, -55});
    //leftBar->addChildAtPosition(btn, Anchor::Top, {3, -55});
    if (menuIndex < 2) {
        m_buttonMenu->addChildAtPosition(
            btn,
            Anchor::TopLeft,
            {55, (-60.F) + (-25 * menuIndex)}
            // -85
        );
    } else {
        // -25
        /*m_categoryButtons->addChildAtPosition(
            btn,
            Anchor::TopLeft,
            {55, (-120.F) + (-25 * (menuIndex - 2))}
            // -85
        );*/ 
        m_categoryButtons->addChild(btn);
        m_categoryButtons->updateLayout();
    }
}

void ObjectWorkshop::onClickObject(CCObject* sender) {
    m_topCommentsListener.getFilter().cancel();
    auto menuItem = static_cast<CCMenuItemSpriteExtra*>(sender); // how could this go wrong
    if (auto objectItem = typeinfo_cast<ObjectItem*>(menuItem->getChildren()->objectAtIndex(0))) {
        auto objectData = objectItem->getData();
        m_currentObject = objectData;
        m_currentMenu = 2;
        rightBg->setVisible(false);
        bottomPageLabel->setVisible(false);
        obj_backBtn->setVisible(true);

        auto middleBg = CCScale9Sprite::create("square02_small.png");
        middleBg->setOpacity(25);
        middleBg->setContentSize({295, 245});
        objectInfoNode->addChildAtPosition(middleBg, Anchor::Center, {0, -8});

        auto menu = CCMenu::create();
        menu->setPosition({10, 218});
        auto infoSpr = CCSprite::createWithSpriteFrameName("GJ_infoIcon_001.png");
        infoSpr->setScale(0.7F);
        auto infoBtn = CCMenuItemSpriteExtra::create(
            infoSpr,
            this,
            menu_selector(ObjectWorkshop::onDescBtn)
        );
        infoBtn->setPositionX(140);
        menu->addChild(infoBtn);

        auto zoomInSpr = CCSprite::createWithSpriteFrameName("GJ_zoomInBtn_001.png");
        zoomInSpr->setScale(0.4F);
        auto zoomInBtn = CCMenuItemSpriteExtra::create(
            zoomInSpr, this, menu_selector(ObjectWorkshop::onZoomIn)
        );
        auto zoomOutSpr = CCSprite::createWithSpriteFrameName("GJ_zoomOutBtn_001.png");
        zoomOutSpr->setScale(0.4F);
        auto zoomOutBtn = CCMenuItemSpriteExtra::create(
            zoomOutSpr, this, menu_selector(ObjectWorkshop::onZoomOut)
        );
        auto resetZoomSpr = CCSprite::createWithSpriteFrameName("GJ_updateBtn_001.png");
        resetZoomSpr->setScale(0.35F);
        auto resetZoomBtn = CCMenuItemSpriteExtra::create(
            resetZoomSpr, this, menu_selector(ObjectWorkshop::onResetZoom)
        );

        zoomInBtn->setPosition({108, -84});
        zoomOutBtn->setPosition({122, -84});
        resetZoomBtn->setPosition({136, -84});

        menu->addChild(zoomInBtn);
        menu->addChild(zoomOutBtn);
        menu->addChild(resetZoomBtn);

        objectInfoNode->addChild(menu);
    
        auto menu2 = CCMenu::create();
        auto trashSpr = CCSprite::createWithSpriteFrameName("GJ_trashBtn_001.png");
        trashSpr->setScale(0.4F);
        auto trashBtn = CCMenuItemSpriteExtra::create(
            trashSpr, this, menu_selector(ObjectWorkshop::onTrashBtn)
        );
        auto editSpr = CCSprite::createWithSpriteFrameName("GJ_editBtn_001.png");
        editSpr->setScale(0.225F);
        auto editBtn = CCMenuItemSpriteExtra::create(
            editSpr, this, menu_selector(ObjectWorkshop::onEditBtn)
        );
        auto acceptSpr = CCSprite::createWithSpriteFrameName("GJ_likeBtn_001.png");
        acceptSpr->setScale(0.4F);
        auto acceptBtn = CCMenuItemSpriteExtra::create(
            acceptSpr, this, menu_selector(ObjectWorkshop::onVerifyBtn)
        );
        auto rejectSpr = CCSprite::createWithSpriteFrameName("GJ_dislikeBtn_001.png");
        rejectSpr->setScale(0.4F);
        auto rejectBtn = CCMenuItemSpriteExtra::create(
            rejectSpr, this, menu_selector(ObjectWorkshop::onRejectBtn)
        );
        auto reportSpr = CCSprite::createWithSpriteFrameName("GJ_reportBtn_001.png");
        reportSpr->setScale(0.4F);
        auto reportBtn = CCMenuItemSpriteExtra::create(
            reportSpr, this, menu_selector(ObjectWorkshop::onReportBtn)
        );
        if (m_user.account_id == m_currentObject.authorAccId || m_user.role == 3) {
            menu2->addChild(editBtn);
            menu2->addChild(trashBtn);
        }
        if (m_user.role >= 2 && m_currentObject.pending) {
            menu2->addChild(acceptBtn);
            menu2->addChild(rejectBtn);
        }
        if (m_user.account_id != m_currentObject.authorAccId && m_authenticated) {
            menu2->addChild(reportBtn);
        }

        menu2->setContentSize({18, 80});
        menu2->setPosition({16, 177});
        objectInfoNode->addChild(menu2);

        menu2->setLayout(
            RowLayout::create()
                ->setAxisAlignment(AxisAlignment::Even)
                ->setCrossAxisAlignment(AxisAlignment::End)
                ->setAutoScale(true)
                ->setCrossAxisOverflow(false)
                ->setDefaultScaleLimits(.1f, 1.f)
                ->setGap(3)
                ->setGrowCrossAxis(true)
        );
        menu2->updateLayout();

        auto leftSideBG1 = CCScale9Sprite::create("square02_small.png");
        leftSideBG1->setOpacity(25);
        leftSideBG1->setScaleX(0.65F);
        leftSideBG1->setContentSize({ 27.F, 82.F });

        previewBG = ExtPreviewBG::create(objectData.objectString);
        middleBg->addChildAtPosition(previewBG, Anchor::TopLeft, {90, -50});
        middleBg->addChildAtPosition(leftSideBG1, Anchor::TopLeft, {16, -50 });

        auto rightSideBG1 = CCScale9Sprite::create("square02_small.png");
        rightSideBG1->setOpacity(25);
        rightSideBG1->setContentSize({128, 82});
        middleBg->addChildAtPosition(rightSideBG1, Anchor::TopRight, {-72, -50});

        auto objTitleLabel = CCLabelBMFont::create(objectData.name.c_str(), "bigFont.fnt");
        auto objAuthorLabel = CCLabelBMFont::create(fmt::format("By {}", objectData.authorName).c_str(), "goldFont.fnt");
        objTitleLabel->limitLabelWidth(70.0F, 0.7F, 0.25F);
        objAuthorLabel->limitLabelWidth(80.0F, 0.7F, 0.25F);

        auto textAreaDesc = MDTextArea::create(objectData.description, {240, 80});
        textAreaDesc->setScale(0.5F);
        auto vLine1 = CCSprite::createWithSpriteFrameName("edit_vLine_001.png");
        vLine1->setRotation(90);

        rightSideBG1->addChildAtPosition(objTitleLabel, Anchor::Top, {0, -10});
        rightSideBG1->addChildAtPosition(objAuthorLabel, Anchor::Top, {0, -20});
        rightSideBG1->addChildAtPosition(textAreaDesc, Anchor::Bottom, {0, 25});
        rightSideBG1->addChildAtPosition(vLine1, Anchor::Center, { 0, 10 });

        auto rightSideBG2 = CCScale9Sprite::create("square02_small.png");
        rightSideBG2->setOpacity(25);
        rightSideBG2->setContentSize({128, 55});
        middleBg->addChildAtPosition(rightSideBG2, Anchor::Right, {-72, -42});

        auto rateItLbl = CCLabelBMFont::create("Rate It!", "bigFont.fnt");
        rateItLbl->setScale(0.35F);
        rightSideBG2->addChildAtPosition(rateItLbl, Anchor::Top, {0, -10});

        auto starsNode = ObjectItem::createClickableStars(this, menu_selector(ObjectWorkshop::onRateBtn));
        starsNode->setLayout(
            RowLayout::create()
                ->setAxisAlignment(AxisAlignment::Center)
                ->setAutoScale(false)
                ->setCrossAxisOverflow(false)
                ->setGap(3)
                ->setGrowCrossAxis(true)
        );
        starsNode->updateLayout();
        starsNode->setScale(1.55F);
        rightSideBG2->addChildAtPosition(starsNode, Anchor::TopLeft, {64, -33});

        std::ostringstream oss;
        oss << std::fixed << std::setprecision(2) << objectData.rating;
        auto ratingLbl = CCLabelBMFont::create(fmt::format("{} ({})", oss.str(), objectData.ratingCount).c_str(), "bigFont.fnt");

        ccColor3B ratingColor = { 255, 255, 0 };
        // yes i used chat jippity because i cant think at 2 am
        if (objectData.rating != 5.0) {
            if (objectData.rating >= 5.0) {
                // Yellow
                ratingColor = {255, 255, 0};
            } else if (objectData.rating >= 4.0) {
                // Green to Yellow
                double t = (objectData.rating - 4.0);
                ratingColor = {static_cast<GLubyte>(255 * t), 255, 0};
            } else if (objectData.rating >= 3.0) {
                // Orange to Green
                double t = (objectData.rating - 3.0);
                ratingColor = {static_cast<GLubyte>(255 * (1 - t)), static_cast<GLubyte>(165 * (1 - t)), 0};
            } else if (objectData.rating >= 2.0) {
                // Reddish Orange to Orange
                double t = (objectData.rating - 2.0);
                ratingColor = {255, static_cast<GLubyte>(70 + 95 * t), 0};
            } else {
                // Red to Reddish Orange
                double t = (objectData.rating - 1.0);
                ratingColor = {255, static_cast<GLubyte>(70 * t), 0};
            }
        }
        for (int i = 0; i < 4; i++) {
            // why doesnt CCFontSprite EXIST!?
            auto fontSpr = static_cast<CCSprite*>(ratingLbl->getChildren()->objectAtIndex(i));
            fontSpr->setColor(ratingColor);
        }

        ratingLbl->setScale(0.275F);
        rightSideBG2->addChildAtPosition(ratingLbl, Anchor::Bottom, {3, 10});
        //rightSideBG2->addChildAtPosition(ratingCountLbl, Anchor::Right, {-34, 10});

        auto vLine2 = CCSprite::createWithSpriteFrameName("edit_vLine_001.png");
        vLine2->setRotation(90);
        rightSideBG2->addChildAtPosition(vLine2, Anchor::Top, {0, -20});

        auto leftSideBG2 = CCScale9Sprite::create("square02_small.png");
        leftSideBG2->setOpacity(25);
        leftSideBG2->setContentSize({145, 36});
        middleBg->addChildAtPosition(leftSideBG2, Anchor::Left, {80, 10});
        
        //std::count(str.begin(), str.end(), ';');
        
        auto tagsLabel = CCLabelBMFont::create("Tags:", "goldFont.fnt");
        auto tagsNode = FiltersPopup::createTags(m_currentObject.tags);
        tagsLabel->setScale(0.41F);
        tagsLabel->setAnchorPoint({0, 0.5});
        leftSideBG2->addChildAtPosition(tagsLabel, Anchor::Left, {6, 7});
        //leftSideBG2->addChildAtPosition(tagsNode, Anchor::Left, {35, 6});
        leftSideBG2->addChildAtPosition(tagsNode, Anchor::Left, {35, 0});
        
        auto rightSideBG3 = CCScale9Sprite::create("square02_small.png");
        rightSideBG3->setOpacity(25);
        rightSideBG3->setContentSize({125, 36});
        auto infoLabels = CCLabelBMFont::create(
            fmt::format(
                "Objects: {}\nVersion: {}\n{}",
                std::count(m_currentObject.objectString.begin(), m_currentObject.objectString.end(), ';'),
                m_currentObject.version,
                (m_currentObject.pending) ? "Status: PENDING" : ""
                ).c_str(),
            "bigFont.fnt"
        );
        infoLabels->setContentHeight(100);
        infoLabels->setAnchorPoint({0, 1.0});
        infoLabels->setScale(0.31F);
        rightSideBG3->addChildAtPosition(infoLabels, Anchor::TopLeft, {3, -2});
        middleBg->addChildAtPosition(rightSideBG3, Anchor::Right, {-72, 10});

        auto bottomRightBG = CCScale9Sprite::create("square02_small.png");
        bottomRightBG->setOpacity(25);
        bottomRightBG->setContentSize({125, 36});
        middleBg->addChildAtPosition(bottomRightBG, Anchor::Bottom, {75, 30});

        auto downloadSpr = CCSprite::createWithSpriteFrameName("GJ_downloadBtn_001.png");
        downloadSpr->setScale(0.6F);
        auto unfavSpr = CCSprite::createWithSpriteFrameName("gj_heartOff_001.png");
        unfavSpr->setScale(0.8F);
        auto favSpr = CCSprite::createWithSpriteFrameName("gj_heartOn_001.png");
        favSpr->setScale(0.8F);
        auto commentSpr = CCSprite::createWithSpriteFrameName("GJ_chatBtn_001.png");
        commentSpr->setScale(0.5F);

        auto favBtn = CCMenuItemToggler::create(unfavSpr, favSpr, this, menu_selector(ObjectWorkshop::onFavBtn));
        favBtn->toggle(Utils::arrayIncludes(m_user.favorites, m_currentObject.id));

        auto downloadBtn = CCMenuItemSpriteExtra::create(downloadSpr, this, menu_selector(ObjectWorkshop::onDownloadBtn));
        
        auto commentBtn = CCMenuItemSpriteExtra::create(commentSpr, this, menu_selector(ObjectWorkshop::onCommentBtn));

        commentBtn->setID("commentbtn"_spr);
        favBtn->setID("favbtn"_spr);
        downloadBtn->setID("downloadbtn"_spr);
        m_buttonMenu->addChildAtPosition(favBtn, Anchor::BottomRight, {-85, 45}); // -63
        m_buttonMenu->addChildAtPosition(downloadBtn, Anchor::BottomRight, {-125, 45}); // -100
        m_buttonMenu->addChildAtPosition(commentBtn, Anchor::BottomRight, {-45, 45});

        downloadsLabel = CCLabelBMFont::create(std::to_string(m_currentObject.downloads).c_str(), "bigFont.fnt");
        favoritesLabel = CCLabelBMFont::create(std::to_string(m_currentObject.favorites).c_str(), "bigFont.fnt");
        commentsLabel = CCLabelBMFont::create("N/A", "bigFont.fnt");
        downloadsLabel->setAnchorPoint({0.5, 1});
        favoritesLabel->setAnchorPoint({0.5, 1});
        commentsLabel->setAnchorPoint({0.5, 1});
        downloadsLabel->limitLabelWidth(25.0F, 0.3F, 0.1F);
        favoritesLabel->limitLabelWidth(25.0F, 0.3F, 0.1F);
        commentsLabel->limitLabelWidth(25.0F, 0.3F, 0.1F);

        bottomRightBG->addChildAtPosition(downloadsLabel, Anchor::Bottom, {-40,11});
        bottomRightBG->addChildAtPosition(favoritesLabel, Anchor::Bottom, {0,11});
        bottomRightBG->addChildAtPosition(commentsLabel, Anchor::Bottom, {40,11});

        auto bottomLeftBG1 = CCScale9Sprite::create("square02_small.png");
        bottomLeftBG1->setOpacity(25);
        bottomLeftBG1->setContentSize({145, 95});
        
        auto commentLabel = CCLabelBMFont::create("Comments (N/A)", "bigFont.fnt");
        commentLabel->setAnchorPoint({0.5, 1.0});
        commentLabel->limitLabelWidth(180.F, 0.4F, 0.3F);
        
        auto vLine3 = CCSprite::createWithSpriteFrameName("edit_vLine_001.png");
        vLine3->setRotation(90);

        bottomLeftBG1->addChildAtPosition(commentLabel, Anchor::Top, {0, -1});
        bottomLeftBG1->addChildAtPosition(vLine3, Anchor::Top, {0, -17});

        middleBg->addChildAtPosition(bottomLeftBG1, Anchor::Bottom, {-68, 60});

        auto loadCommentsSpr = ButtonSprite::create("Load Comments", 180.F, 0, 0.6F, true, "bigFont.fnt", "GJ_button_01.png", 25.F);
        loadCommentsSpr->setScale(0.6F);
        auto loadCommentsBtn = CCMenuItemSpriteExtra::create(
            loadCommentsSpr,
            this,
            menu_selector(ObjectWorkshop::onLoadComments)
        );
        loadCommentsBtn->setID("loadcomments"_spr);
        m_buttonMenu->addChildAtPosition(loadCommentsBtn, Anchor::Bottom, {-15, 70});

        auto leftArrowSpr = CCSprite::createWithSpriteFrameName("GJ_arrow_01_001.png");
        auto rightArrowSpr = CCSprite::createWithSpriteFrameName("GJ_arrow_01_001.png");
        rightArrowSpr->setFlipX(true);
        leftArrowSpr->setScale(0.4F);
        rightArrowSpr->setScale(0.4F);
        auto leftArrowBtn = CCMenuItemSpriteExtra::create(leftArrowSpr, this, menu_selector(ObjectWorkshop::onLeftCommentPage));
        auto rightArrowBtn = CCMenuItemSpriteExtra::create(rightArrowSpr, this, menu_selector(ObjectWorkshop::onRightCommentPage));
        leftArrowBtn->setPosition({7, -137});
        rightArrowBtn->setPosition({132, -137});
        menu->addChild(leftArrowBtn);
        menu->addChild(rightArrowBtn);

        leftArrowBtn->setEnabled(false);
        rightArrowBtn->setEnabled(false);

        auto pageLabel = CCLabelBMFont::create("", "goldFont.fnt");
        pageLabel->setScale(0.4F);
        bottomLeftBG1->addChildAtPosition(pageLabel, Anchor::Bottom, {0, -5});

        m_topCommentsListener.bind([this, commentLabel, bottomLeftBG1, leftArrowBtn, rightArrowBtn, pageLabel] (web::WebTask::Event* e) {
            bottomLeftBG1->removeChildByID("commentscroll"_spr);
            if (web::WebResponse* value = e->getValue()) {
                if (m_currentMenu == 2) {
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
                        commentLabel->setString(fmt::format("Comments ({})", c_total.value()).c_str());
                        commentsLabel->setString(std::to_string(c_total.value()).c_str());
                    }
                    if (c_page) {
                        m_currentObject.commentPage = c_page.value();
                    }
                    if (c_pageAmount) {
                        m_currentObject.maxCommentPage = c_pageAmount.value();
                    }
                    commentLabel->limitLabelWidth(180.F, 0.4F, 0.3F);
                    auto nodeArray = CCArray::create();
                    
                    auto scrollLayer = ScrollLayerExt::create({ 0, 0, 145.0F, 75.F }, true);
                    auto content = CCMenu::create();
                    content->setZOrder(2);
                    content->setContentWidth(145.0F);
                    scrollLayer->m_contentLayer->setContentSize({
                        content->getContentSize().width,
                        50.F * array.size()
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
                            }, m_currentObject, m_user, [this]() {
                                ObjectWorkshop::onLoadComments(nullptr);
                            }));
                        }
                    }
                    if (array.size() <= 1) {
                        scrollLayer->setTouchEnabled(false);
                    }
                    content->setLayout(
                        ColumnLayout::create()
                            ->setAxisAlignment(AxisAlignment::End)
                            ->setCrossAxisOverflow(false)
                            ->setGap(5)
                            ->setGrowCrossAxis(true)
                            ->setAxisReverse(true)
                    );
                    content->setAnchorPoint({0.5, 0});
                    content->setPosition({72, 0});
                    //auto listView = static_cast<BoomListView*>(CustomListView::create(nodeArray,BoomListType::Comment,75.F,145.F)); // 7
                    //bottomLeftBG1->addChildAtPosition(listView, Anchor::BottomLeft);
                    scrollLayer->setID("commentscroll"_spr);
                    bottomLeftBG1->addChildAtPosition(scrollLayer, Anchor::BottomLeft);
                    scrollLayer->moveToTop();
                    scrollLayer->fixTouchPrio();
                    pageLabel->setString(fmt::format("Page {} of {}", m_currentObject.commentPage, m_currentObject.maxCommentPage).c_str());
                }
            } else if (web::WebProgress* progress = e->getProgress()) {
                // The request is still in progress...
            } else if (e->isCancelled()) {
                log::error("Request was cancelled.");
            }
        });
    }
}

void ObjectWorkshop::onLeftCommentPage(CCObject* sender) {
    if (m_currentObject.commentPage > 1) {
        m_currentObject.commentPage--;
        //m_pageInput->setString(std::to_string(m_currentPage).c_str());
        ObjectWorkshop::onLoadComments(sender);
    }
}
void ObjectWorkshop::onRightCommentPage(CCObject* sender) {
    if ((m_currentObject.commentPage + 1) <= m_currentObject.maxCommentPage) {
        m_currentObject.commentPage++;
        //m_pageInput->setString(std::to_string(m_currentPage).c_str());
        ObjectWorkshop::onLoadComments(sender);
    }
}

void ObjectWorkshop::onLoadComments(CCObject*) {
    m_buttonMenu->removeChildByID("loadcomments"_spr);
    web::WebRequest req = web::WebRequest();
    m_topCommentsListener.setFilter(req.get(fmt::format("{}/objects/{}/comments?limit=10&page={}", HOST_URL, m_currentObject.id, m_currentObject.commentPage)));
}

void ObjectWorkshop::onZoomIn(CCObject*) {
    if (previewBG != nullptr) previewBG->updateZoom(0.2F);
}
void ObjectWorkshop::onZoomOut(CCObject*) {
    if (previewBG != nullptr) previewBG->updateZoom(-0.2F);
}
void ObjectWorkshop::onResetZoom(CCObject*) {
    if (previewBG != nullptr) previewBG->resetZoom();
}

void ObjectWorkshop::onCommentBtn(CCObject*) {
    CommentPopup::create(m_currentObject, [this]() {
        ObjectWorkshop::onLoadComments(nullptr);
    })->show();
}

void ObjectWorkshop::onFavBtn(CCObject*) {
    if (!m_authenticated) return FLAlertLayer::create("Error", "You cannot favorite levels as you are <cy>not authenticated!</c>", "OK")->show();
    m_favoriteListener.getFilter().cancel();
    m_favoriteListener.bind([this] (web::WebTask::Event* e) {
        if (web::WebResponse* value = e->getValue()) {
            auto jsonRes = value->json().unwrapOrDefault();
            if (!jsonRes.is_object()) return log::error("Response isn't object.");
            auto isError = jsonRes.try_get<std::string>("error");
            if (isError) return Notification::create(isError->c_str(), NotificationIcon::Error)->show();
            auto message = jsonRes.try_get<std::string>("message");
            if (message) {
                Notification::create(message->c_str(), NotificationIcon::Success)->show();
            } else {
                log::error("Unknown response, expected message. {}", jsonRes.dump());
                Notification::create("Got an unknown response, check logs for details.", NotificationIcon::Warning)->show();
            }
            m_currentObject.favorited = !m_currentObject.favorited;
            if (m_currentObject.favorited) {
                m_currentObject.favorites++;
                m_user.favorites.push_back(m_currentObject.id);
            } else {
                m_currentObject.favorites--;
                auto it = std::find(m_user.favorites.begin(), m_user.favorites.end(), m_currentObject.id);
                if (it != m_user.favorites.end()) { 
                    m_user.favorites.erase(it); 
                }
            }
            if (m_currentMenu == 2 && favoritesLabel != nullptr) favoritesLabel->setString(std::to_string(m_currentObject.favorites).c_str());
            return;
        } else if (web::WebProgress* progress = e->getProgress()) {
            // The request is still in progress...
        } else if (e->isCancelled()) {
            log::error("Request was cancelled.");
        }
    });
    web::WebRequest req = web::WebRequest();
    auto myjson = matjson::Value();
    myjson.set("token", m_token);
    req.header("Content-Type", "application/json");
    req.bodyJSON(myjson);
    m_favoriteListener.setFilter(req.post(fmt::format("{}/objects/{}/favorite", HOST_URL, m_currentObject.id)));
}

void ObjectWorkshop::actuallyDownload() {
    if (auto gameManager = GameManager::sharedState()) {
        if (auto editorUI = CustomObjects::get()) {
            gameManager->addNewCustomObject(m_currentObject.objectString);
            editorUI->reloadCustomItems();
            Notification::create("Downloaded object!", NotificationIcon::Success)->show();
        }
    }
}

void ObjectWorkshop::onDownloadBtn(CCObject*) {
    if (Utils::arrayIncludes(m_user.downloaded, m_currentObject.id)) {
        geode::createQuickPopup(
            "Info",
            "You have already <cg>downloaded this object</c>!\nWould you like to <cy>download anyways</c>?",
            "No",
            "Yes",
            [this](auto, bool btn2) {
                if (btn2) {
                    actuallyDownload();
                }
            },
            true,
            true
        );
    } else {
        if (m_currentObject.pending) return actuallyDownload();
        m_downloadListener.getFilter().cancel();
        m_downloadListener.bind([this] (web::WebTask::Event* e) {
            if (web::WebResponse* value = e->getValue()) {
                if (value->json().has_error()) return log::error("Response is not JSON.");
                auto jsonRes = value->json().unwrapOrDefault();
                if (!jsonRes.is_object()) return log::error("Response isn't object.");
                auto isError = jsonRes.try_get<std::string>("error");
                if (isError) return Notification::create(isError->c_str(), NotificationIcon::Error)->show();
                if (m_currentMenu == 2 && downloadsLabel != nullptr) downloadsLabel->setString(std::to_string(m_currentObject.downloads).c_str());
                return;
            } else if (web::WebProgress* progress = e->getProgress()) {
                // The request is still in progress...
            } else if (e->isCancelled()) {
                log::error("Request was cancelled.");
            }
        });
        m_currentObject.downloads++;
        m_user.downloaded.push_back(m_currentObject.id);
        actuallyDownload();
        web::WebRequest req = web::WebRequest();
        auto myjson = matjson::Value();
        myjson.set("token", m_token);
        req.header("Content-Type", "application/json");
        req.bodyJSON(myjson);
        m_downloadListener.setFilter(req.post(fmt::format("{}/objects/{}/download", HOST_URL, m_currentObject.id)));
    }
}

void ObjectWorkshop::onRateBtn(CCObject* sender) {
    if (!m_authenticated) return FLAlertLayer::create("Error", "You cannot rate levels as you are <cy>not authenticated!</c>", "OK")->show();
    m_rateListener.getFilter().cancel();
    auto menuItem = static_cast<CCMenuItemSpriteExtra*>(sender);
    auto menu = static_cast<CCMenu*>(menuItem->getParent());
    for (int i = 0; i < 5; i++) {
        auto item = typeinfo_cast<CCMenuItemSpriteExtra*>(menu->getChildByID(fmt::format("{}", i + 1)));
        if (item) {
            auto node = static_cast<CCNode*>(item->getChildren()->objectAtIndex(0));
            auto starFull = static_cast<CCSprite*>(node->getChildByID("full"));
            auto starEmpty = static_cast<CCSprite*>(node->getChildByID("empty"));
            starFull->setVisible(false);
            starEmpty->setVisible(false);
            if (std::stoi(item->getID()) <= std::stoi(menuItem->getID())) {
                starFull->setVisible(true);
            } else {
                starEmpty->setVisible(true);
            }
        }
    }
    m_rateListener.bind([this] (web::WebTask::Event* e) {
        if (web::WebResponse* value = e->getValue()) {
            auto jsonRes = value->json().unwrapOrDefault();
            if (!jsonRes.is_object()) return log::error("Response isn't object.");
            auto isError = jsonRes.try_get<std::string>("error");
            if (isError) return Notification::create(isError->c_str(), NotificationIcon::Error)->show();
            Notification::create("Rated!", NotificationIcon::Success)->show();
            return;
        } else if (web::WebProgress* progress = e->getProgress()) {
            // The request is still in progress...
        } else if (e->isCancelled()) {
            log::error("Request was cancelled.");
        }
    });
    web::WebRequest req = web::WebRequest();
    auto myjson = matjson::Value();
    myjson.set("token", m_token);
    myjson.set("stars", std::stoi(menuItem->getID()));
    req.header("Content-Type", "application/json");
    req.bodyJSON(myjson);
    m_rateListener.setFilter(req.post(fmt::format("{}/objects/{}/rate", HOST_URL, m_currentObject.id)));
}

void ObjectWorkshop::onDescBtn(CCObject*) {
    FLAlertLayer::create(
        "Object Info",
        fmt::format(
            "<cp>ID</c>: {}\n<cg>Uploader</c>: {}\n<cy>Uploaded</c>: {}\n<cy>Updated</c>: {}\n<cl>Version</c>: {}\n<co>Tags</c>: {}",
            m_currentObject.id,
            m_currentObject.authorName,
            m_currentObject.created,
            m_currentObject.updated,
            m_currentObject.version,
            fmt::join(m_currentObject.tags, ", ")
        ).c_str(),
        "OK"
    )->show();
}

void ObjectWorkshop::onTrashBtn(CCObject*) {
    geode::createQuickPopup(
        "Warning",
        "Are you sure you want to <cy>delete this object</c>?\nYou <cr>cannot go back from this</c>!",
        "No",
        "Yes",
        [this](auto, bool btn2) {
            if (btn2) {
                m_deleteListener.getFilter().cancel();
                m_deleteListener.bind([this] (web::WebTask::Event* e) {
                    if (web::WebResponse* value = e->getValue()) {
                        auto jsonRes = value->json().unwrapOrDefault();
                        if (!jsonRes.is_object()) return log::error("Response isn't object.");
                        auto isError = jsonRes.try_get<std::string>("error");
                        if (isError) return Notification::create(isError->c_str(), NotificationIcon::Error)->show();
                        auto message = jsonRes.try_get<std::string>("message");
                        if (message) {
                            Notification::create(message->c_str(), NotificationIcon::Success)->show();
                        } else {
                            log::error("Unknown response, expected message. {}", jsonRes.dump());
                            Notification::create("Got an unknown response, check logs for details.", NotificationIcon::Warning)->show();
                        }
                        onBackBtn(nullptr);
                        RegenCategory();
                        return;
                    } else if (web::WebProgress* progress = e->getProgress()) {
                        // The request is still in progress...
                    } else if (e->isCancelled()) {
                        log::error("Request was cancelled.");
                    }
                });
                web::WebRequest req = web::WebRequest();
                auto myjson = matjson::Value();
                myjson.set("token", m_token);
                req.header("Content-Type", "application/json");
                req.bodyJSON(myjson);
                m_deleteListener.setFilter(req.post(fmt::format("{}/objects/{}/delete", HOST_URL, m_currentObject.id)));
            }
        },
        true,
        true
    );
}
void ObjectWorkshop::onEditBtn(CCObject*) {
    EditPopup::create(m_currentObject, m_availableTags, m_user)->show();
}
void ObjectWorkshop::onVerifyBtn(CCObject*) {
    geode::createQuickPopup(
        "Warning",
        "Are you sure you want to <cy>accept this object</c>?",
        "No",
        "Yes",
        [this](auto, bool btn2) {
            if (btn2) {
                m_reviewListener.getFilter().cancel();
                m_reviewListener.bind([this] (web::WebTask::Event* e) {
                    if (web::WebResponse* value = e->getValue()) {
                        auto jsonRes = value->json().unwrapOrDefault();
                        if (!jsonRes.is_object()) return log::error("Response isn't object.");
                        auto isError = jsonRes.try_get<std::string>("error");
                        if (isError) return Notification::create(isError->c_str(), NotificationIcon::Error)->show();
                        auto message = jsonRes.try_get<std::string>("message");
                        if (message) {
                            Notification::create(message->c_str(), NotificationIcon::Success)->show();
                        } else {
                            log::error("Unknown response, expected message. {}", jsonRes.dump());
                            Notification::create("Got an unknown response, check logs for details.", NotificationIcon::Warning)->show();
                        }
                        if (m_currentMenu == 2) {
                            onBackBtn(nullptr);
                            RegenCategory();
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
                myjson.set("token", m_token);
                req.header("Content-Type", "application/json");
                req.bodyJSON(myjson);
                m_reviewListener.setFilter(req.post(fmt::format("{}/objects/{}/accept", HOST_URL, m_currentObject.id)));
            }
        },
        true,
        true
    );
}
void ObjectWorkshop::onRejectBtn(CCObject*) {
    geode::createQuickPopup(
        "Warning",
        "Are you sure you want to <cy>reject this object</c>?",
        "No",
        "Yes",
        [this](auto, bool btn2) {
            if (btn2) {
                m_reviewListener.getFilter().cancel();
                m_reviewListener.bind([this] (web::WebTask::Event* e) {
                    if (web::WebResponse* value = e->getValue()) {
                        auto jsonRes = value->json().unwrapOrDefault();
                        if (!jsonRes.is_object()) return log::error("Response isn't object.");
                        auto isError = jsonRes.try_get<std::string>("error");
                        if (isError) return Notification::create(isError->c_str(), NotificationIcon::Error)->show();
                        auto message = jsonRes.try_get<std::string>("message");
                        if (message) {
                            Notification::create(message->c_str(), NotificationIcon::Success)->show();
                        } else {
                            log::error("Unknown response, expected message. {}", jsonRes.dump());
                            Notification::create("Got an unknown response, check logs for details.", NotificationIcon::Warning)->show();
                        }
                        if (m_currentMenu == 2) {
                            onBackBtn(nullptr);
                            RegenCategory();
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
                myjson.set("token", m_token);
                req.header("Content-Type", "application/json");
                req.bodyJSON(myjson);
                m_reviewListener.setFilter(req.post(fmt::format("{}/objects/{}/reject", HOST_URL, m_currentObject.id)));
            }
        },
        true,
        true
    );
}
void ObjectWorkshop::onReportBtn(CCObject*) {
    ReportPopup::create(m_currentObject)->show();
}

void ObjectWorkshop::onBackBtn(CCObject*) {
    if (m_currentMenu == 0) return;
    if (bottomPageLabel != nullptr) bottomPageLabel->setVisible(true);
    m_currentMenu = 0;
    rightBg->setVisible(true);
    obj_backBtn->setVisible(false);
    objectInfoNode->removeAllChildrenWithCleanup(true);
    m_buttonMenu->removeChildByID("favbtn"_spr);
    m_buttonMenu->removeChildByID("commentbtn"_spr);
    m_buttonMenu->removeChildByID("downloadbtn"_spr);
    m_buttonMenu->removeChildByID("loadmorebtn"_spr);
    m_buttonMenu->removeChildByID("uploadbtn"_spr);
    m_buttonMenu->removeChildByID("rulesbtn"_spr);
    m_buttonMenu->removeChildByID("tagbtn"_spr);
    m_buttonMenu->removeChildByID("loadcomments"_spr);
}

template <typename T>
T* clonePointer(const T* original) {
    if (original == nullptr) {
        return nullptr;  // Handle null pointers.
    }
    return new T(*original);  // Use the copy constructor to create a new object.
}

void ObjectWorkshop::onUploadBtn(CCObject*) {
    m_filterTags.clear();
    m_currentMenu = 1;
    rightBg->setVisible(false);
    bottomPageLabel->setVisible(false);
    obj_backBtn->setVisible(true);

    auto middleBg = CCScale9Sprite::create("square02_small.png");
    middleBg->setOpacity(25);
    middleBg->setContentSize({295, 220});
    objectInfoNode->addChildAtPosition(middleBg, Anchor::Center, {0, 5});

    auto previewBG = CCScale9Sprite::create("square02_small.png");
    previewBG->setOpacity(60);
    previewBG->setContentSize({ 295.F - 20.F, 82.F });
    auto previewLabel = CCLabelBMFont::create("Select an Object", "goldFont.fnt");
    previewLabel->setScale(0.425F);
    previewBG->addChildAtPosition(previewLabel, Anchor::Top, {0,-8});
    middleBg->addChildAtPosition(previewBG, Anchor::Top, {0, -50});

    auto bottomBg = CCScale9Sprite::create("square02_small.png");
    bottomBg->setOpacity(25);
    bottomBg->setContentSize({275, 120});
    middleBg->addChildAtPosition(bottomBg, Anchor::Center, {0, -45});

    m_objName = TextInput::create(300.0F, "Object Name", "bigFont.fnt");
    m_objName->setScale(0.8);
    m_objName->setMaxCharCount(64);
    bottomBg->addChildAtPosition(m_objName, Anchor::Top, {0, -20});
#ifndef GEODE_IS_ANDROID32
    auto textArea = TextArea::create("", "chatFont.fnt", 1.0F, 270.0F, {0.5, 0.5}, 20.0F, true);
    //             TextArea::create(&local_64,"chatFont.fnt",,0x439d8000,this_03,0x41a00000,1);
#endif
    m_objDesc = TextInput::create(270.0F, "Description [Optional]", "chatFont.fnt");
#ifndef GEODE_IS_ANDROID32
    m_objDesc->getInputNode()->addTextArea(textArea);
    m_objDesc->getInputNode()->m_cursor->setOpacity(0);
#endif
    m_objDesc->getBGSprite()->setContentSize({520.0F, 100.0F});
    m_objDesc->setMaxCharCount(300);
    m_objDesc->setCommonFilter(CommonFilter::Any);
    bottomBg->addChildAtPosition(m_objDesc, Anchor::Center, {0, -3});
#ifndef GEODE_IS_ANDROID32
    m_objDesc->setCallback(
        [this, textArea](std::string p0) {
            m_objDesc->getInputNode()->m_placeholderLabel->setOpacity((p0.empty()) ? 255 : 0);
            textArea->setScale(Utils::calculateScale(p0, 50, 300, 0.9F, 0.35F));
            textArea->m_width = 220.0F / Utils::calculateScale(p0, 50, 300, 1.0F, 0.32F);
            textArea->setString(m_objDesc->getInputNode()->getString());
            //textArea->setString(p0.data());
        }
    );
#endif
    auto rulesSpr = ButtonSprite::create("Rules", "bigFont.fnt", "GJ_button_03.png");
    rulesSpr->setScale(0.8F);
    auto uploadSpr = ButtonSprite::create("Upload", "bigFont.fnt", "GJ_button_01.png");
    uploadSpr->setScale(0.8F);
    auto uploadBtn = CCMenuItemSpriteExtra::create(
        uploadSpr,
        this,
        menu_selector(ObjectWorkshop::onUpload)
    );
    auto rulesBtn = CCMenuItemSpriteExtra::create(
        rulesSpr,
        this,
        menu_selector(ObjectWorkshop::onRulesBtn)
    );
    uploadBtn->setID("uploadbtn"_spr);
    rulesBtn->setID("rulesbtn"_spr);
    auto filterSpr = ButtonSprite::create(
        CCSprite::createWithSpriteFrameName("GJ_filterIcon_001.png"),
        30,
        0,
        .0F,
        1.0F,
        false,
        "GJ_button_04.png",
        false
    );
    filterSpr->setScale(0.75F);
    auto filterBtn = CCMenuItemSpriteExtra::create(
        filterSpr,
        this,
        menu_selector(ObjectWorkshop::onUploadFilterBtn)
    );
    filterBtn->setID("tagbtn"_spr);

    m_buttonMenu->addChildAtPosition(filterBtn, Anchor::Bottom, {-60, 57});
    m_buttonMenu->addChildAtPosition(uploadBtn, Anchor::BottomRight, {-85, 57});
    m_buttonMenu->addChildAtPosition(rulesBtn, Anchor::BottomRight, {-195, 57});
    //bottomBg->addChildAtPosition(textArea, Anchor::Center, {0, -20});
    if (auto editor = CustomObjects::get()) {
        auto scrollLayer = ScrollLayerExt::create({ 0, 0, 275.0F, 280.0F }, true);
        scrollLayer->setContentSize({275.0F, 60.0F});
        scrollLayer->setAnchorPoint({0.5, 1.0});
        auto content = CCMenu::create();
        content->setScale(0.675F);
        content->setZOrder(2);
        content->setPositionX(20);
        content->registerWithTouchDispatcher();
        
        scrollLayer->m_contentLayer->addChild(content);
        scrollLayer->setTouchEnabled(true);

        CCArrayExt<CreateMenuItem*> customItems = editor->createCustomItems();
        int size = customItems.size() - 4;
        for (int i = 0; i < size; i++) {
            customItems[i]->setID(fmt::format("{}", i));
            if (i > 17) {
                customItems[i]->setEnabled(false);
            }
            content->addChild(customItems[i]);
        }
        previewBG->addChild(scrollLayer);
        content->setLayout(
            RowLayout::create()
                ->setAxisAlignment(AxisAlignment::Start)
                ->setCrossAxisAlignment(AxisAlignment::End)
                ->setAutoScale(true)
                ->setCrossAxisOverflow(false)
                ->setGap(5)
                ->setGrowCrossAxis(true)
        );
        content->setContentSize({400.0F, 400.0F});
        content->setAnchorPoint({0.5, 1.0});
        content->setPosition({137, 280});
        //content->setContentSize({265.0F, 230.0F});
        content->updateLayout();
        scrollLayer->moveToTop();
        scrollLayer->fixTouchPrio();
        scrollLayer->setCallbackMove([size, content]() {
            if (content == nullptr) return;
            for (int i = 0; i < size; i++) {
                if (auto child = typeinfo_cast<CreateMenuItem*>(content->getChildByID(fmt::format("{}", i)))) {
                    child->setEnabled(false);
                }
            }
        });
        scrollLayer->setCallbackEnd([size, content, scrollLayer]() {
            if (content == nullptr) return;
            for (int i = 0; i < size; i++) {
                if (auto child = typeinfo_cast<CreateMenuItem*>(content->getChildByID(fmt::format("{}", i)))) {
                    float contentYPos = scrollLayer->m_contentLayer->getPositionY();
                    float childYPos = (child->getPositionY());

                    child->setEnabled(!Utils::isInScrollSnapRange(contentYPos, childYPos));

                    //float index = -(contentYPos + 220) / 30.F;
                    //float lower_bound = 380.F + index * 35.F;
                    //float upper_bound = lower_bound - 35;

                    //child->setEnabled(upper_bound <= childYPos <= lower_bound);

                    // 60 
                }
            }
            if (scrollLayer->m_contentLayer->getPositionY() > -220.F) {
                scrollLayer->m_contentLayer->setPositionY(Utils::getSnappedYPosition(scrollLayer->m_contentLayer->getPositionY(), 300)); // or 290
            }
        });
    }
}

void ObjectWorkshop::onSearchBtn(CCObject*) {
    if (m_searchInput != nullptr) {
        if (m_searchInput->getString().empty()) return FLAlertLayer::create("Error", "You must enter in a <cy>search query</c>!", "OK")->show();
        isSearching = true;
        RegenCategory();
    }
}

void ObjectWorkshop::onReloadBtn(CCObject*) {
    onBackBtn(nullptr);
    RegenCategory();
}

void ObjectWorkshop::onFilterBtn(CCObject*) {
    FiltersPopup::create(m_availableTags, m_filterTags, false, [this](std::unordered_set<std::string> selectedTags) {
        if (m_filterTags != selectedTags) {
            m_filterTags = selectedTags;
            RegenCategory();
        }
    })->show();
}

void ObjectWorkshop::onPendingBtn(CCObject*) {
    isSearching = false;
    m_currentPage = 1;
    m_pageInput->setString("1");
    currentMenuIndexGD = -1;
    RegenCategory();
}

void ObjectWorkshop::onUploadFilterBtn(CCObject*) {
    FiltersPopup::create(m_availableTags, m_filterTags, true, [this](std::unordered_set<std::string> selectedTags) {
        m_filterTags = selectedTags;
    })->show();
}
void ObjectWorkshop::onUpload(CCObject*) {
    if (auto editor = EditorUI::get()) {
        if (auto gameManager = GameManager::sharedState()) {
            if (m_filterTags.size() > 5) return FLAlertLayer::create("Error", "You cannot set more than <cy>5 tags</c>!", "OK")->show();
            if (m_objName == nullptr || m_objDesc == nullptr) return FLAlertLayer::create("Error", "Couldn't find <cy>input nodes</c>", "OK")->show();
            if (m_objName != nullptr && m_objName->getString().empty()) return FLAlertLayer::create("Error", "You must enter in the <cy>object name</c>!", "OK")->show();
            ObjectData obj = {
                0,
                m_objName->getString(),
                "[No description provided]"
            };
            if (editor->m_selectedObjectIndex < 0) { // genius robert!
                obj.objectString = gameManager->stringForCustomObject(editor->m_selectedObjectIndex);
            }
            if (obj.objectString == "") return FLAlertLayer::create("Error", "You must <cy>select an object</c>!", "OK")->show();
            if (m_objDesc != nullptr && m_objDesc->getString().length() > 0) {
                obj.description = m_objDesc->getString();
            }
            obj.tags = m_filterTags;
            obj_backBtn->setVisible(false);
            objectInfoNode->removeAllChildrenWithCleanup(true);
            m_buttonMenu->removeChildByID("favbtn"_spr);
            m_buttonMenu->removeChildByID("loadcomments"_spr);
            m_buttonMenu->removeChildByID("commentbtn"_spr);
            m_buttonMenu->removeChildByID("downloadbtn"_spr);
            m_buttonMenu->removeChildByID("loadmorebtn"_spr);
            m_buttonMenu->removeChildByID("uploadbtn"_spr);
            m_buttonMenu->removeChildByID("rulesbtn"_spr);
            m_buttonMenu->removeChildByID("tagbtn"_spr);
            loadingCircle = LoadingCircle::create();
            loadingCircle->setPosition({-20, -20});
            loadingCircle->setParentLayer(m_buttonMenu);
            loadingCircle->show();

            m_uploadListener.getFilter().cancel();
            m_buttonMenu->setEnabled(false);
            m_uploadListener.bind([this] (web::WebTask::Event* e) {
                if (web::WebResponse* value = e->getValue()) {
                    m_currentMenu = 0;
                    rightBg->setVisible(true);
                    bottomPageLabel->setVisible(true);
                    auto jsonRes = value->json().unwrapOrDefault();
                    if (!jsonRes.is_object()) return log::error("Response isn't object.");
                    auto isError = jsonRes.try_get<std::string>("error");
                    if (isError) return Notification::create(isError->c_str(), NotificationIcon::Error)->show();
                    Notification::create("Uploaded Object! It is now pending!", NotificationIcon::Success)->show();
                    if (loadingCircle != nullptr) loadingCircle->fadeAndRemove();
                    m_buttonMenu->setEnabled(true);
                    return;
                } else if (web::WebProgress* progress = e->getProgress()) {
                    // The request is still in progress...
                } else if (e->isCancelled()) {
                    m_currentMenu = 0;
                    log::error("Request was cancelled.");
                    rightBg->setVisible(true);
                    bottomPageLabel->setVisible(true);
                    m_buttonMenu->setEnabled(true);
                    if (loadingCircle != nullptr) loadingCircle->fadeAndRemove();
                }
            });
            m_filterTags.clear();
            web::WebRequest req = web::WebRequest();
            auto myjson = matjson::Value();
            myjson.set("token", m_token);
            myjson.set("name", obj.name);
            myjson.set("description", obj.description);
            myjson.set("tags", obj.tags);
            myjson.set("data", obj.objectString);
            req.header("Content-Type", "application/json");
            req.bodyJSON(myjson);
            m_uploadListener.setFilter(req.post(fmt::format("{}/objects/upload", HOST_URL)));
        }
    }
}

void ObjectWorkshop::onClose(CCObject* sender) {
    m_listener0.getFilter().cancel();
    m_listener1.getFilter().cancel();
    m_listener2.getFilter().cancel();
    m_rateListener.getFilter().cancel();
    m_editListener.getFilter().cancel();
    m_favoriteListener.getFilter().cancel();
    m_downloadListener.getFilter().cancel();
    m_deleteListener.getFilter().cancel();
    m_reviewListener.getFilter().cancel();
    m_uploadListener.getFilter().cancel();
    m_tagsListener.getFilter().cancel();

    m_commentListener.getFilter().cancel();
    m_commentsListener.getFilter().cancel();
    m_topCommentsListener.getFilter().cancel();
    this->setKeypadEnabled(false);
    this->setTouchEnabled(false);
    this->removeFromParentAndCleanup(true);
    Popup::onClose(sender);
}

void ObjectWorkshop::keyDown(cocos2d::enumKeyCodes key) {
    if (key == cocos2d::enumKeyCodes::KEY_Escape) {
        if (rightBg->isVisible()) {
            this->onClose(nullptr);
        } else {
            this->onBackBtn(nullptr);
        }
        return;
    }
    if (key == cocos2d::enumKeyCodes::KEY_Space) return;
    return FLAlertLayer::keyDown(key);
}

void ObjectWorkshop::keyBackClicked() {
    if (rightBg->isVisible()) {
        this->onClose(nullptr);
    } else {
        this->onBackBtn(nullptr);
    }
}
