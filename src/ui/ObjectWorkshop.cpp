#include "../config.hpp"
#include "ObjectWorkshop.hpp"
#include "Geode/ui/Scrollbar.hpp"
#include "popups/WarningPopup.hpp"
#include "popups/includes.h"
#include "admin/AdminPopup.hpp"
#include "../nodes/CategoryButton.hpp"
#include "../utils.hpp"

int currentMenuIndexGD = 2;
std::unordered_set<std::string> g_availableTags;

static float s_scrollHeight = 227.f;

bool ObjectWorkshop::init(bool authenticated) {
    if (!Popup::init(425.f, 290.f)) return false;
    m_leftSide = true;
    if (currentMenuIndexGD == -1) currentMenuIndexGD = 2;
    if (auto editor = EditorUI::get()) {
        m_editorLayer = editor->m_editorLayer;
        m_inEditor = true;
    } else {
        m_editorLayer = LevelEditorLayer::create(GJGameLevel::create(), false);
        GameManager::sharedState()->fadeInMenuMusic();
        m_inEditor = false;
    }
    m_user.authenticated = authenticated;
    //m_authenticated = false;

    web::WebRequest req = web::WebRequest();
    req.userAgent(USER_AGENT);
    auto certValid = Mod::get()->getSettingValue<bool>("cert-valid");
    if (!certValid) {
        req.certVerification(certValid);
    }
    async::spawn(
        req.get(fmt::format("{}/objects/tags", HOST_URL)),
        [this](web::WebResponse value) {
            if (value.code() >= 500 && !value.ok()) {
                Notification::create("A server error occured. Check logs for info.", NotificationIcon::Error)->show();
                log::error("{}", value.string().unwrapOrDefault());
                return;
            }
            auto jsonRes = value.json().unwrapOrDefault();
            if (jsonRes.isObject()) {
                auto isError = jsonRes.get("error");
                if (isError.isOk()) return Notification::create(isError.unwrap().asString().unwrapOrDefault(), NotificationIcon::Error)->show();
                log::error("{}", jsonRes.dump());
                return Notification::create("(Tags) Expected array, but got object.", NotificationIcon::Error)->show();
            }
            auto jsonArr = jsonRes.asArray().unwrap();
            g_availableTags = Utils::arrayToUnorderedSet<std::string>(jsonArr);
            log::debug("Current available tags: {}", g_availableTags);
        }
    );

    Build<CCNode>::create().anchorPoint(0.5, 0.5).contentSize(360, s_scrollHeight).store(objectInfoNode).parentAtPos(m_mainLayer, Anchor::Center, {53, -4});
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

    Build<CCLabelBMFont>::create("Object Workshop", "goldFont.fnt").scale(0.675F).zOrder(5).parentAtPos(m_mainLayer, Anchor::Top, {(m_leftSide) ? 10.f : -24.f, -14});
    Build<CCScale9Sprite>::create("square02_small.png").opacity(60).contentSize({27, 120}).anchorPoint(0.5, 1).parentAtPos(m_buttonMenu, (m_leftSide) ? Anchor::TopLeft : Anchor::TopRight, {(m_leftSide) ? 24.f : -27.f, -25});

    Build<CCSprite>::createSpriteName("GJ_profileButton_001.png").scale(0.7f).intoMenuItem([this]() {
        if (!m_user.authenticated) return FLAlertLayer::create("Error", "You cannot view your profile as you are <cy>not authenticated!</c>", "OK")->show();
        onClickUser(m_user.account_id);
    }).parentAtPos(m_buttonMenu, (m_leftSide) ? Anchor::TopLeft : Anchor::TopRight, {(m_leftSide) ? 23.f : -27.f, -26});

    if (authenticated) {
        Build<CCMenu>::create().anchorPoint({0.5, 1}).contentSize({30, 100}).layout(
            ColumnLayout::create()
                ->setAxisReverse(true)
                ->setAutoScale(false)
                ->setCrossAxisOverflow(false)
                ->setAxisAlignment(AxisAlignment::Even)
        ).store(m_categoryButtonsTop).parentAtPos(m_buttonMenu, (m_leftSide) ? Anchor::TopLeft : Anchor::TopRight, {(m_leftSide) ? 24.f : -27.f, -45});
        createCategoryBtn("GJ_downloadBtn_001.png", true, 10, true); // [My Recently Downloaded Objects] [10]
        createCategoryBtn("gj_heartOn_001.png", true, 1, true); // [My Favorited Objects] [1]
        createCategoryBtn("rankIcon_1_001.png", true, 9, true); // [Leaderboard] (Creator Points, Downloads, etc) [??]
    }

    Build<CCMenu>::create().anchorPoint({0.5, 1}).contentSize({30, 135}).layout(
        ColumnLayout::create()
            ->setAxisReverse(true)
            ->setAutoScale(false)
            ->setCrossAxisOverflow(false)
            ->setAxisAlignment(AxisAlignment::Even)
    ).store(m_categoryButtons).parentAtPos(m_buttonMenu, (m_leftSide) ? Anchor::Left : Anchor::Right, {(m_leftSide) ? 24.f : -27.f, -4});

    createCategoryBtn("GJ_downloadsIcon_001.png", true, 2, false); // 0 [Top Downloads]
    createCategoryBtn("GJ_likesIcon_001.png", true, 4, false); // 2 [Most Liked]
    createCategoryBtn("GJ_sTrendingIcon_001.png", true, 3, false); // 1 [Most Popular]
    createCategoryBtn("GJ_sRecentIcon_001.png", true, 6, false); // 4 [Most Recent]

    Build<CCMenu>::create().anchorPoint({0.5, 1}).contentSize({360, 25}).layout(
        RowLayout::create()
            ->setAutoScale(false)
            ->setCrossAxisOverflow(false)
            ->setAxisAlignment(AxisAlignment::Between)
    ).store(m_topMenu).parentAtPos(m_buttonMenu, Anchor::Top, {13, -30});

    float searchInputWidth = 360.f; // 340
    m_searchInput = TextInput::create(searchInputWidth, "Search...");
    m_searchInput->setMaxCharCount(64);
    m_searchInput->setCommonFilter(CommonFilter::Any);
    m_searchInput->setScale(0.775F);
    m_searchInput->setAnchorPoint({ 0, .5f });
    m_searchInput->getBGSprite()->setContentHeight(45);
    m_searchInput->setTextAlign(TextInputAlign::Left);
    m_topMenu->addChild(m_searchInput);
    if (auto item = typeinfo_cast<CCMenuItemSpriteExtra*>(m_categoryButtons->getChildByID(fmt::format("category-{}"_spr, currentMenuIndexGD)))) {
        static_cast<CategoryButton*>(item->getChildren()->objectAtIndex(0))->setIndicatorState(true);
    }
    if (auto item = typeinfo_cast<CCMenuItemSpriteExtra*>(m_categoryButtonsTop->getChildByID(fmt::format("category-{}"_spr, currentMenuIndexGD)))) {
        static_cast<CategoryButton*>(item->getChildren()->objectAtIndex(0))->setIndicatorState(true);
    }
    Build<ButtonSprite>::create(
        CCSprite::createWithSpriteFrameName("GJ_filterIcon_001.png"),
        30,
        0,
        .0F,
        1.0F,
        false,
        "GJ_button_04.png",
        false
    ).scale(.4f).intoMenuItem([this]() {
        FiltersPopup::create(g_availableTags, m_filterTags, m_selectedFeatured, m_user.role, currentMenuIndexGD == 7, [this](std::unordered_set<std::string> selectedTags, bool featured, bool pending, bool reports) {
            bool regenCate = false;
            int newID = 0;
            if (pending && currentMenuIndexGD != 7) {
                newID = 7;
            } else if (reports && currentMenuIndexGD != 8) {
                newID = 8;
            } else if ((!pending && currentMenuIndexGD == 7) || (!reports && currentMenuIndexGD == 8)) {
                newID = 2;
            }
            if (m_filterTags != selectedTags) {
                m_filterTags = selectedTags;
                regenCate = true;
            }
            if (featured != m_selectedFeatured) {
                m_selectedFeatured = featured;
                regenCate = true;
            }
            if (regenCate || newID > 0) {
                if (newID > 0) {
                    isSearching = false;
                    onBackBtn(nullptr);
                    if (newID != currentMenuIndexGD) {
                        if (auto oldItem = typeinfo_cast<CCMenuItemSpriteExtra*>(m_categoryButtonsTop->getChildByID(fmt::format("category-{}"_spr, currentMenuIndexGD)))) {
                            static_cast<CategoryButton*>(oldItem->getChildren()->objectAtIndex(0))->setIndicatorState(false);
                        }
                        if (auto oldItem = typeinfo_cast<CCMenuItemSpriteExtra*>(m_categoryButtons->getChildByID(fmt::format("category-{}"_spr, currentMenuIndexGD)))) {
                            static_cast<CategoryButton*>(oldItem->getChildren()->objectAtIndex(0))->setIndicatorState(false);
                        }
                        if (auto newItem = typeinfo_cast<CCMenuItemSpriteExtra*>(m_categoryButtonsTop->getChildByID(fmt::format("category-{}"_spr, newID)))) {
                            static_cast<CategoryButton*>(newItem->getChildren()->objectAtIndex(0))->setIndicatorState(true);
                        }
                        currentMenuIndexGD = newID;
                    }
                }
                m_currentPage = 1;
                m_pageInput->setString("1");
                RegenCategory();
            }
        })->show();
    }).parent(m_topMenu);
    Build<CCSprite>::createSpriteName("gj_findBtn_001.png").scale(.5f).intoMenuItem([this]() {
        if (m_searchInput != nullptr) {
            if (m_searchInput->getString().empty()) return FLAlertLayer::create("Error", "You must enter in a <cy>search query</c>!", "OK")->show();
            if (currentMenuIndexGD < 2) return FLAlertLayer::create("Error", "You cannot search in <cy>My Objects</c> or <cy>Favorites</c>! Please select another category.", "OK")->show();
            m_currentPage = 1;
            m_pageInput->setString("1");
            isSearching = true;
            RegenCategory();
        }
    }).parent(m_topMenu);
    rightBg = CCScale9Sprite::create("square02_small.png");
    rightBg->setOpacity(60);
    rightBg->setZOrder(1);
    rightBg->setContentSize({360, s_scrollHeight}); // 295, 225
    m_mainLayer->addChildAtPosition(rightBg, Anchor::Center, {(m_leftSide) ? 13.f : -21.f, -23.f});

    m_scrollLayer = ScrollLayerExt::create({ 0, 0, 340.F, s_scrollHeight }, true); // 360
    m_scrollLayer->setPositionX(10);
    m_content = CCMenu::create();
    m_content->setZOrder(2);
    m_content->setPositionX(45);
    m_content->registerWithTouchDispatcher();

    // auto topG = Build<CCSprite>::createSpriteName("d_gradient_01_001.png").opacity(0).zOrder(1).flipY(true).color(0,0,0).anchorPoint(0,1).scaleToMatchX(rightBg->getContentWidth()).parent(rightBg).posY(227).collect();
    // auto bottomG = Build<CCSprite>::createSpriteName("d_gradient_01_001.png").opacity(120).zOrder(1).color(0,0,0).anchorPoint(0,0).scaleToMatchX(rightBg->getContentWidth()).parent(rightBg).collect();
    //
    // m_scrollLayer->setCallbackMove([topG, bottomG, this]() {
    //     if (m_scrollLayer->m_contentLayer->getPositionY() >= -130) {
    //         topG->runAction(CCSequence::create(CCFadeTo::create(0.2f, 120.f), nullptr));
    //     } else {
    //         topG->runAction(CCSequence::create(CCFadeTo::create(0.2f, 0.f), nullptr));
    //     }
    //     if (!(m_scrollLayer->m_contentLayer->getPositionY() >= -10)) {
    //         bottomG->runAction(CCSequence::create(CCFadeTo::create(0.2f, 120.f), nullptr));
    //     } else {
    //         bottomG->runAction(CCSequence::create(CCFadeTo::create(0.2f, 0.f), nullptr));
    //     }
    // });

    m_scrollLayer->m_contentLayer->addChild(m_content);

    m_scrollLayer->setTouchEnabled(true);
    m_scrollLayer->setMouseEnabled(true);

    rightBg->addChild(m_scrollLayer);
    //rightBg->setVisible(false);

    if (authenticated) {
        auto token = Mod::get()->getSettingValue<std::string>("token");
        m_token = token;
        m_listener.cancel();
        web::WebRequest req = web::WebRequest();
        req.userAgent(USER_AGENT);
        auto certValid = Mod::get()->getSettingValue<bool>("cert-valid");
        if (!certValid) {
            req.certVerification(certValid);
        }
        auto myjson = matjson::Value();
        myjson.set("token", m_token);
        req.header("Content-Type", "application/json");
        req.bodyJSON(myjson);
        m_listener.spawn(
            req.post(fmt::format("{}/user/@me", HOST_URL)),
            [this](web::WebResponse value) {
                auto jsonRes = value.json().unwrapOrDefault();
                if (Utils::notifError(jsonRes)) return;
                auto userRes = matjson::Serialize<UserData>::fromJson(jsonRes);
                if (userRes.isOk()) {
                    m_user = userRes.unwrap();
                    m_user.authenticated = true;
                    log::debug("Set user information.");
                    if (jsonRes.contains("warning") && jsonRes.contains("cases")) {
                        auto caseJsonArray = jsonRes.get("cases").unwrap().asArray().unwrap();
                        if (caseJsonArray.size() > 0) {
                            hasWarning = true;
                            auto caseJsonRes = caseJsonArray[0];
                            log::debug("User has warning, setting info for it.");
                            matjson::Value defaultVal;
                            caseData = {
                                (int)caseJsonRes.get("case_id").unwrapOr(defaultVal).asInt().unwrapOrDefault(),
                                (int)jsonRes.get("warning").unwrap().asInt().unwrapOrDefault(),
                                static_cast<CaseType>(caseJsonRes.get("case_type").unwrapOr(defaultVal).asInt().unwrapOrDefault()),
                                caseJsonRes.get("reason").unwrap().asString().unwrapOr("No reason provided."),
                                "N/A"
                            };
                            if (!shownObjects) {
                                WarningPopup::create(caseData, [this]() {
                                    acknowledgedWarning = true;
                                })->show();
                                hasWarning = false;
                                shownWarning = true;
                            }
                        }
                    }
                } else {
                    log::error("Something went wrong when getting keys from the users object. {}", userRes.err());
                    Notification::create("Couldn't parse user object.", NotificationIcon::Warning)->show();
                }
                if (m_user.role >= 2) {
                    //createCategoryBtn("Pending", 7);
                    if (m_user.role == 3) {
                        //createCategoryBtn("Reports", 8);
                    }
                }
            }
        );
    }
    Build<CCSprite>::createSpriteName("GJ_arrow_01_001.png").scale(0.8f).intoMenuItem([this]() {
        if (m_currentPage > 1) {
            m_currentPage--;
        } else {
            m_currentPage = m_maxPage;
        }
        m_pageInput->setString(std::to_string(m_currentPage).c_str());
        RegenCategory();
    }).parentAtPos(m_buttonMenu, Anchor::Left, {(m_leftSide) ? 54.f : 16.f, -27});
    Build<CCSprite>::createSpriteName("GJ_arrow_01_001.png").scale(0.8f).flipX(true).intoMenuItem([this]() {
        if ((m_currentPage + 1) <= m_maxPage) {
            m_currentPage++;
        } else {
            m_currentPage = 1;
        }
        m_pageInput->setString(std::to_string(m_currentPage).c_str());
        RegenCategory();
    }).parentAtPos(m_buttonMenu, Anchor::Right, {(m_leftSide) ? -30.f : -54.f, -27});
    Build<CCSprite>::createSpriteName("GJ_updateBtn_001.png").scale(0.8f).intoMenuItem([this]() {
        onBackBtn(nullptr);
        RegenCategory();
    }).parentAtPos(m_buttonMenu, Anchor::TopRight, {22, -16});

    m_pageInput = TextInput::create(65.0F, "Page...");
    m_pageInput->setString("1");
    m_pageInput->setMaxCharCount(8);
    m_pageInput->setScale(0.525F);
    m_pageInput->setCommonFilter(CommonFilter::Uint);
    m_pageInput->setAnchorPoint({1, 0.5f});
    m_pageInput->setDelegate(this);
    m_topMenu->addChild(m_pageInput);
    if (m_leftSide) {
        m_closeBtn->updateAnchoredPosition(Anchor::TopLeft, {-22, -16});
        auto scrollBar = Scrollbar::create(m_scrollLayer);
        if (auto bg = scrollBar->getChildByType<CCScale9Sprite>(0)) {
            bg->setOpacity(60);
        }
        m_mainLayer->addChildAtPosition(scrollBar, (m_leftSide) ? Anchor::Right : Anchor::Left, {(m_leftSide) ? -11.f : 11.f, -23});
    }
    m_topMenu->updateLayout();

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
    auto idStr = item->getID().view();
    //idStr.remove_prefix(idStr.length() - 1);
    idStr.remove_prefix(fmt::format("category-"_spr).length());
    int id = numFromString<int>(idStr).unwrapOrDefault();;
    if (id != currentMenuIndexGD) {
        if (auto oldItem = typeinfo_cast<CCMenuItemSpriteExtra*>(m_categoryButtonsTop->getChildByID(fmt::format("category-{}"_spr, currentMenuIndexGD)))) {
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
    // this definitely wont go wrong
    if (!shownWarning && hasWarning) {
        Loader::get()->queueInMainThread([this]() {
            WarningPopup::create(caseData, [this]() {
                acknowledgedWarning = true;
                RegenCategory();
            })->show();
        });
        shownWarning = true;
        return;
    }
    if (!acknowledgedWarning && hasWarning && shownWarning) return;
    Loader::get()->queueInMainThread([this]() {
        int myItems = 0;
        int items = 0;
        log::debug("RegenCategory ({},{})", currentMenuIndexGD, m_currentPage);

        loadingCircle = LoadingCircle::create();
        m_buttonMenu->removeChildByID("retrybtn"_spr);
        m_content->removeAllChildrenWithCleanup(true);
        myUploadsBar = CCNode::create();
        auto myUploadsLabel = CCLabelBMFont::create("My Uploads", "bigFont.fnt");
        myUploadsLabel->limitLabelWidth(90.0F, 0.8F, 0.2F);
        auto mUbarLeft = BreakLine::create(90.F);
        auto mUbarRight = BreakLine::create(90.F);
        mUbarRight->setAnchorPoint({1, 0});
        myUploadsBar->addChildAtPosition(mUbarLeft, Anchor::Left);
        myUploadsBar->addChildAtPosition(myUploadsLabel, Anchor::Center);
        myUploadsBar->addChildAtPosition(mUbarRight, Anchor::Right);

        myUploadsBar->setContentSize({305, 25});
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

        myUploadsMenu = CCMenu::create();

        myUploadsMenu->setPosition({125,-5});
        myUploadsMenu->setContentSize(m_scrollLayer->getContentSize());
        myUploadsMenu->setAnchorPoint({0.5, 0});
        myUploadsMenu->setLayout(
            RowLayout::create()
                ->setAxisAlignment(AxisAlignment::Center)
                ->setAutoScale(false)
                ->setCrossAxisOverflow(false)
                ->setGap(8)
                ->setGrowCrossAxis(true)
        );
        auto uploadSpr = CCSprite::create("upload.png"_spr);
        uploadSpr->setScale(0.625F);
        auto uploadBtn = CCMenuItemSpriteExtra::create(uploadSpr, this, menu_selector(ObjectWorkshop::onUploadBtn));
        myUploadsMenu->addChild(uploadBtn);
        m_content->addChild(myUploadsMenu);
        myUploadsMenu->updateLayout();

        Build<CCMenu>::create().with([](auto node) {
            int barSize = 40;
            switch (currentMenuIndexGD) {
                case -2:
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
                    barSize = 120;
                    break;
            }
            // because idk the math formula
            auto barLeft = BreakLine::create(barSize, 2.f);
            auto barRight = BreakLine::create(barSize, 2.f);
            barRight->setAnchorPoint({1, 1});
            barLeft->setAnchorPoint({0, 1});
            node->addChildAtPosition(barLeft, Anchor::Left);
            Build<CCLabelBMFont>::create(Utils::menuIndexToString(currentMenuIndexGD).c_str(), "bigFont.fnt").limitLabelWidth(120.F, 0.475F, 0.25F).parentAtPos(node, Anchor::Center);
            node->addChildAtPosition(barRight, Anchor::Right);
        }).contentSize(305, 25).anchorPoint(0.5, 0.5).store(m_categoryBar).updateLayout().parent(m_content).posX(124);

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

        myUploadsBar->setPosition({140, 138});
        m_categoryBar->setPosition({140, 74});
        categoryItems->setPosition({138, -105});

        m_scrollLayer->m_contentLayer->setContentSize({
            m_content->getContentSize().width,
            m_content->getContentSize().height - 90
        });
        m_content->setPositionY(80);
        m_scrollLayer->setTouchEnabled(false);
        
        if (currentMenuIndexGD != 0) {
            myUploadsBar->setVisible(false);
            if (currentMenuIndexGD != -1) {
                myUploadsMenu->setVisible(false);
                m_content->setPositionY(140);
            }
        }
        m_scrollLayer->moveToTop();
        cocos::handleTouchPriority(m_buttonMenu);
        load();
    });
    setKeyboardEnabled(true);
}

void ObjectWorkshop::onRetryBtn(CCObject*) {
    ObjectWorkshop::RegenCategory();
}

void ObjectWorkshop::handleRequest1(web::WebResponse value) {
    log::debug("Finished request for listener 1.");
    if (!value.ok()) {
        auto errorText = value.string().unwrapOrDefault();
        log::error("Failed to retrieve data from server: {}", errorText);
        auto errorLabel = CCLabelBMFont::create("Error fetching objects", "bigFont.fnt");
        errorLabel->setScale(0.6F);
        auto retrySpr = ButtonSprite::create("Retry", "bigFont.fnt", "GJ_button_04.png", .8f);
        retrySpr->setScale(0.6F);
        auto retryBtn = CCMenuItemSpriteExtra::create(retrySpr, this, menu_selector(ObjectWorkshop::onRetryBtn));
        retryBtn->setID("retrybtn"_spr);
        auto detailsLabel = CCLabelBMFont::create(value.string().unwrapOrDefault().c_str(), "chatFont.fnt");
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
    auto jsonRes = value.json().unwrapOrDefault();
    if (Utils::notifError(jsonRes)) return;
    if (!value.ok()) return Notification::create("An unknown error occured.", NotificationIcon::Error)->show();
    auto arrayRes = jsonRes.get("results");
    std::vector<matjson::Value> array;
    if (arrayRes.isOk()) {
        array = arrayRes.unwrap().asArray().unwrap();
    }
    for (auto item : array) {
        auto objRes = matjson::Serialize<ObjectData>::fromJson(item);
        if (objRes.isErr()) {
            log::error("One of the cells could not be properly parsed! {}", objRes.err());
            continue;
        }
        auto objData = objRes.unwrap();
        objData.favorited = Utils::arrayIncludes(m_user.favorites, objData.id);
        if (currentMenuIndexGD == 8 && (item.contains("reports"))) {
            auto reportArrayRes = item.get("reports");
            std::vector<matjson::Value> reportArray;
            if (reportArrayRes.isOk()) {
                reportArray = reportArrayRes.unwrap().asArray().unwrap();
            }
            if (reportArray.size() > 0) {
                for (auto report : reportArray) {
                    auto reportRes = matjson::Serialize<ReportData>::fromJson(report);
                    if (reportRes.isOk()) {
                        objData.reports.push_back(reportRes.unwrap());
                    }
                }
            }
        }
        auto cell = CCMenuItemSpriteExtra::create(ObjectItem::create(m_editorLayer, objData), this, menu_selector(ObjectWorkshop::onClickObject));
        categoryItems->addChild(cell);
    }
    m_buttonMenu->removeChildByID("pageLabelBtn"_spr);
    auto o_page = jsonRes.get("page");
    auto o_pageAmount = jsonRes.get("pageAmount");
    auto o_total = jsonRes.get("total");
    if (o_page.isOk() && o_pageAmount.isOk() && o_total.isOk()) {
        unsigned int total = o_total.unwrap().asInt().unwrapOrDefault();
        Build<CCLabelBMFont>::create(
            fmt::format(
                "Page {} of {}",
                o_page.unwrap().asInt().unwrapOrDefault(),
                o_pageAmount.unwrap().asInt().unwrapOrDefault()
            ).c_str(), "goldFont.fnt"
        ).scale(.425f).id("pageLabel"_spr).store(m_pageLabel).intoMenuItem([total]() {
            FLAlertLayer::create("Page", fmt::format("There are a total of <cy>{} objects</c> based on your filters.", total).c_str(), "OK")->show();
        }).anchorPoint(0.5f, 1).id("pageLabelBtn"_spr).parentAtPos(m_buttonMenu, Anchor::TopRight, {(m_leftSide) ? -48.f : -95.f, -6});
        m_maxPage = o_pageAmount.unwrap().asInt().unwrapOrDefault();
    }
    m_scrollLayer->setTouchEnabled(true);
    m_amountItems = array.size();
    if ((currentMenuIndexGD != 0 && currentMenuIndexGD != -1)) {
        if (m_amountItems > 6) {
            categoryItems->setContentSize({
                categoryItems->getContentWidth(),
                355.F // 310
            });
            myUploadsBar->setPosition({125, 138});
            m_categoryBar->setPosition({125, 74});
            categoryItems->setPosition({140, -299});

            m_scrollLayer->m_contentLayer->setContentSize({
                m_content->getContentSize().width,
                m_content->getContentSize().height + 70
            });
            m_content->setPositionY(300);
        } else {
            categoryItems->setContentSize({
                categoryItems->getContentWidth(),
                225.F
            });
            myUploadsBar->setPosition({125, 138});
            m_categoryBar->setPosition({125, 74});
            categoryItems->setPosition({138, -160});

            m_scrollLayer->m_contentLayer->setContentSize({
                m_content->getContentSize().width,
                m_content->getContentSize().height - 70
            });
            m_content->setPositionY(160);
        }

        myUploadsMenu->removeAllChildrenWithCleanup(true);
        myUploadsBar->setPosition({125, 285});
        myUploadsMenu->updateLayout();
        myUploadsBar->updateLayout();
        m_categoryBar->updateLayout();
        categoryItems->updateLayout();
        if (loadingCircle != nullptr) loadingCircle->fadeAndRemove();
        cocos::handleTouchPriority(m_content);
        m_scrollLayer->fixTouchPrio();
        categoryItems->setVisible(true);
        myUploadsMenu->setVisible(true);
    }
    m_scrollLayer->moveToTop();
    if (m_user.authenticated && currentMenuIndexGD == 0) {
        categoryItems->setContentSize({
            categoryItems->getContentWidth(),
            225.F
        });
        web::WebRequest request2 = web::WebRequest();
        auto myjson = matjson::Value();
        myjson.set("token", m_token);
        request2.header("Content-Type", "application/json");
        request2.userAgent(USER_AGENT);
        auto certValid = Mod::get()->getSettingValue<bool>("cert-valid");
        if (!certValid) {
            request2.certVerification(certValid);
        }
        request2.bodyJSON(myjson);
        m_listener2.spawn(
            request2.get(fmt::format("{}/user/@me/objects?page=0&limit=true", HOST_URL)),
            [this](web::WebResponse res) {
                this->handleRequest2(std::move(res));
            }
        );
    } else if (currentMenuIndexGD == -1) {
        auto o_user = jsonRes.get("user");
        UserData user;
        myUploadsMenu->removeAllChildrenWithCleanup(true);
        if (o_user.isOk()) {
            auto p_res = matjson::Serialize<UserData>::fromJson(o_user.unwrap());
            if (p_res.isOk()) {
                user = p_res.unwrap();
            }
        }
        m_currentUser = user;
        auto profileBG = CCScale9Sprite::create("redBG.png"_spr);
        profileBG->setContentSize({275, 70});

        auto profileInnerBG = CCScale9Sprite::create("square02_small.png");
        profileInnerBG->setOpacity(60);
        profileInnerBG->setContentSize({ 260, 40 });

        if (auto gm = GameManager::sharedState()) {
            SimplePlayer* pIcon;
            if (user.icon.size() != 5) {
                pIcon = SimplePlayer::create(1);
            } else {
                pIcon = SimplePlayer::create(user.icon[0]);
                pIcon->setColor(gm->colorForIdx(user.icon[1]));
                pIcon->setSecondColor(gm->colorForIdx(user.icon[2]));
                pIcon->setGlowOutline(gm->colorForIdx(user.icon[3]));
                pIcon->m_hasGlowOutline = user.icon[4] == 1;
                pIcon->updateColors();
            }
            profileInnerBG->addChildAtPosition(pIcon, Anchor::Left, {22, 0});

            auto accName = CCLabelBMFont::create(user.name.c_str(), "goldFont.fnt");
            accName->setAlignment(cocos2d::kCCTextAlignmentCenter);
            accName->limitLabelWidth(100.F, 0.6F, 0.3F);
            profileInnerBG->addChildAtPosition(accName, Anchor::Center, {-35, 9});

            auto uploadsSpr = CCSprite::createWithSpriteFrameName("GJ_hammerIcon_001.png");
            uploadsSpr->setScale(0.5F);
            auto uploadsLabel = CCLabelBMFont::create(std::to_string(o_total.unwrap().asInt().unwrapOrDefault()).c_str(), "bigFont.fnt");
            uploadsLabel->setColor({0, 255, 0});
            uploadsLabel->setAnchorPoint({1, 0.5});
            uploadsLabel->setScale(0.4F);
            auto featuredSpr = CCSprite::createWithSpriteFrameName("GJ_starsIcon_001.png");
            featuredSpr->setScale(0.5F);
            auto featuredLabel = CCLabelBMFont::create(std::to_string(user.featured).c_str(), "bigFont.fnt");
            featuredLabel->setColor({255, 255, 0});
            featuredLabel->setAnchorPoint({1, 0.5});
            featuredLabel->setScale(0.4F);

            profileInnerBG->addChildAtPosition(uploadsSpr, Anchor::Center, {-50, -8});
            profileInnerBG->addChildAtPosition(uploadsLabel, Anchor::Center, {-60, -7});
            profileInnerBG->addChildAtPosition(featuredSpr, Anchor::Center, {-5, -8});
            profileInnerBG->addChildAtPosition(featuredLabel, Anchor::Center, {-15, -7});

            if (user.role >= 2) {
                auto infSpr = CCSprite::createWithSpriteFrameName((user.role == 2) ? "GJ_starBtn_001.png" : "GJ_starBtnMod_001.png");
                infSpr->setScale(0.5F);
                auto infBtn = CCMenuItemSpriteExtra::create(
                    infSpr,
                    this,
                    (user.role == 2) ? menu_selector(ObjectWorkshop::onReviewerInfoBtn) : menu_selector(ObjectWorkshop::onAdminInfoBtn)
                );
                infBtn->setZOrder(1);
                myUploadsMenu->addChildAtPosition(infBtn, Anchor::Left, {12, 30});
            }

            auto vLine = CCSprite::createWithSpriteFrameName("edit_vLine_001.png");
            vLine->setScaleY(0.45F);
            vLine->setScaleX(2.F);
            if (m_user.role == 3) {
                auto adminSpr = CCSprite::createWithSpriteFrameName("accountBtn_settings_001.png");
                adminSpr->setScale(0.75F);
                auto adminBtn = CCMenuItemSpriteExtra::create(adminSpr, this, menu_selector(ObjectWorkshop::onAdminBtn));
                adminBtn->setZOrder(1);
                profileInnerBG->addChildAtPosition(vLine, Anchor::Right, {-85, 0});
                myUploadsMenu->addChildAtPosition(adminBtn, Anchor::Right, {-80, 7});
            } else {
                profileInnerBG->addChildAtPosition(vLine, Anchor::Right, {-55, 0});
            }
        }

        profileBG->addChildAtPosition(profileInnerBG, Anchor::Center, {0, 7});

        auto p_total = jsonRes.get("user_total");
        if (p_total.isOk()) {
            auto downloadSpr = CCSprite::createWithSpriteFrameName("GJ_downloadBtn_001.png");
            downloadSpr->setScale(0.4F);
            auto p_total_downloads = p_total.unwrap().get("downloads").unwrap().asInt().unwrapOrDefault();
            auto downloadsLabel = CCLabelBMFont::create(std::to_string(p_total_downloads).c_str(), "bigFont.fnt");
            downloadsLabel->setAnchorPoint({0, 0.5});
            downloadsLabel->setScale(0.35F);
            profileBG->addChildAtPosition(downloadSpr, Anchor::BottomLeft, {18, 13});
            profileBG->addChildAtPosition(downloadsLabel, Anchor::BottomLeft, {28, 13});
            auto favSpr = CCSprite::createWithSpriteFrameName("gj_heartOn_001.png");
            auto p_total_favs = p_total.unwrap().get("favorites").unwrap().asInt().unwrapOrDefault();
            auto favLabel = CCLabelBMFont::create(std::to_string(p_total_favs).c_str(), "bigFont.fnt");
            favLabel->setAnchorPoint({0, 0.5});
            favLabel->setScale(0.35F);
            favSpr->setScale(0.5F);
            profileBG->addChildAtPosition(favSpr, Anchor::Bottom, {-50, 13});
            profileBG->addChildAtPosition(favLabel, Anchor::Bottom, {-40, 13});
        }

        auto p_avg = jsonRes.get("user_average");
        if (p_avg.isOk()) {
            double averageRating = p_avg.unwrap().get("average_rating").unwrap().asDouble().unwrapOrDefault();
            auto stars = ObjectItem::createStars(averageRating);
            stars->setScale(0.9F);
            stars->setAnchorPoint({0.5, 0});
            std::ostringstream oss;
            oss << std::fixed << std::setprecision(2) << averageRating;
            auto ratingLbl = CCLabelBMFont::create(fmt::format("{} ({})", oss.str(), p_avg.unwrap().get("total_rating_count").unwrap().asInt().unwrapOrDefault()).c_str(), "bigFont.fnt");
            for (int i = 0; i < 4; i++) {
                // why doesnt CCFontSprite EXIST!?
                auto fontSpr = static_cast<CCSprite*>(ratingLbl->getChildren()->objectAtIndex(i));
                fontSpr->setColor(ObjectItem::starColor(averageRating));
            }
            ratingLbl->setScale(0.225F);
            ratingLbl->setAnchorPoint({0.5, 0});
            profileBG->addChildAtPosition(stars, Anchor::BottomRight, {-40, 7});
            profileBG->addChildAtPosition(ratingLbl, Anchor::BottomRight, {-38, 5});
        }

        myUploadsMenu->setLayout(nullptr);

        myUploadsMenu->addChildAtPosition(profileBG, Anchor::Center);

        auto goProfileSpr = CCSprite::createWithSpriteFrameName("GJ_longBtn05_001.png");
        auto goProfileBtn = CCMenuItemSpriteExtra::create(goProfileSpr, this, menu_selector(ObjectWorkshop::onGoProfileBtn));
        myUploadsMenu->addChildAtPosition(goProfileBtn, Anchor::Right, {-42, 7});

        myUploadsMenu->setPosition({125,0});
        m_scrollLayer->setTouchEnabled(true);
        if (m_amountItems > 3) {
            m_categoryBar->setPosition({125, 63});
            categoryItems->setPosition({138, -171});

            m_scrollLayer->m_contentLayer->setContentSize({
                m_content->getContentSize().width,
                m_content->getContentSize().height
            });
            m_content->setPositionY(165);
        } else { // 74
            m_categoryBar->setPosition({125, 63});
            categoryItems->setPosition({138, -116});

            m_scrollLayer->m_contentLayer->setContentSize({
                m_content->getContentSize().width,
                m_content->getContentSize().height - 90
            });
            m_content->setPositionY(75);
            m_scrollLayer->setTouchEnabled(false);
        }
        categoryItems->setVisible(true);
        myUploadsMenu->setVisible(true);
        myUploadsBar->updateLayout();
        m_categoryBar->updateLayout();
        categoryItems->updateLayout();
        cocos::handleTouchPriority(m_content);
        m_scrollLayer->fixTouchPrio();
        if (loadingCircle != nullptr) loadingCircle->fadeAndRemove();
        m_scrollLayer->moveToTop();
    }
    shownObjects = true;
}
void ObjectWorkshop::handleRequest2(web::WebResponse value) {
    log::debug("Finished request for listener 2.");
    auto jsonRes = value.json().unwrapOrDefault();
    if (Utils::notifError(jsonRes)) return;
    if (!value.ok()) return Notification::create("An unknown error occured.", NotificationIcon::Error)->show();
    auto arrayRes = jsonRes.get("results");
    std::vector<matjson::Value> array;
    if (arrayRes.isOk()) {
        array = arrayRes.unwrap().asArray().unwrap();
    }
    if (array.size() > 0) {
        myUploadsMenu->removeAllChildrenWithCleanup(true);
    }
    CCSize defaultContentSize;
    for (auto item : array) {
        auto objRes = matjson::Serialize<ObjectData>::fromJson(item);
        if (objRes.isErr()) {
            log::error("One of the cells could not be properly parsed! {}", objRes.err());
            continue;
        }
        auto objData = objRes.unwrap();
        objData.favorited = Utils::arrayIncludes(m_user.favorites, objData.id);
        auto cell = CCMenuItemSpriteExtra::create(ObjectItem::create(m_editorLayer, objData), this, menu_selector(ObjectWorkshop::onClickObject));
        defaultContentSize = cell->getContentSize();
        myUploadsMenu->addChild(cell);
    }
    if (array.size() > 0) {
        myUploadsMenu->setPosition({125,-36});
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
        myUploadsMenu->setPosition({125,-5});
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
            myUploadsBar->setPosition({125, 138});
            m_categoryBar->setPosition({125, 14});
            categoryItems->setPosition({138, -220});

            m_scrollLayer->m_contentLayer->setContentSize({
                m_content->getContentSize().width,
                m_content->getContentSize().height + 45
            });
            m_content->setPositionY(215);
        } else {
            myUploadsBar->setPosition({125, 138});
            m_categoryBar->setPosition({125, 74});
            categoryItems->setPosition({138, -160});

            m_scrollLayer->m_contentLayer->setContentSize({
                m_content->getContentSize().width,
                m_content->getContentSize().height - 10
            });
            m_content->setPositionY(154);
        }
    } else {
        if (array.size() > 0) {
            myUploadsBar->setPosition({125, 138});
            m_categoryBar->setPosition({125, 14});
            categoryItems->setPosition({138, -165});

            m_scrollLayer->m_contentLayer->setContentSize({
                m_content->getContentSize().width,
                m_content->getContentSize().height - 45
            });
            m_content->setPositionY(125);
        } else {
            myUploadsBar->setPosition({125, 138});
            m_categoryBar->setPosition({125, 74});
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
    m_categoryBar->updateLayout();
    categoryItems->updateLayout();
    cocos::handleTouchPriority(m_content);
    m_scrollLayer->fixTouchPrio();
    if (loadingCircle != nullptr) loadingCircle->fadeAndRemove();
    m_scrollLayer->moveToTop();
}

void ObjectWorkshop::load() {
    log::debug("Loading objects...");
    web::WebRequest request = web::WebRequest();
    request.userAgent(USER_AGENT);
    auto certValid = Mod::get()->getSettingValue<bool>("cert-valid");
    if (!certValid) {
        request.certVerification(certValid);
    }
    m_listener1.cancel();
    m_listener2.cancel();
    categoryItems->setVisible(false);
    myUploadsMenu->setVisible(false);
    auto searchReq = [this](std::string url, bool get = true) {
        web::WebRequest request = web::WebRequest();
        request.userAgent(USER_AGENT);
        if (currentMenuIndexGD != -1 && !get) {
            auto myjson = matjson::Value();
            myjson.set("token", m_token);
            request.header("Content-Type", "application/json");
            request.bodyJSON(myjson);
        }
        auto certValid = Mod::get()->getSettingValue<bool>("cert-valid");
        if (!certValid) {
            request.certVerification(certValid);
        }
        if (get) {
            m_listener1.spawn(
                request.get(url),
                [this](web::WebResponse res) {
                    this->handleRequest1(std::move(res));
                }
            );
        } else {
            m_listener1.spawn(
                request.post(url),
                [this](web::WebResponse res) {
                    this->handleRequest1(std::move(res));
                }
            );
        }
        
    };
    if (isSearching && !m_searchInput->getString().empty()) {
        std::string query = Utils::url_encode(m_searchInput->getString());
        if (m_filterTags.empty()) {
            searchReq(fmt::format("{}/objects/search?query={}&limit={}&page={}", HOST_URL, query, RESULT_LIMIT, m_currentPage), false);
        } else {
            searchReq(
                fmt::format(
                    "{}/objects/search?query={}&page={}&limit={}&tags={}",
                    HOST_URL,
                    query,
                    m_currentPage,
                    RESULT_LIMIT,
                    Utils::url_encode(fmt::format("{}",fmt::join(m_filterTags, ",")))
                ),
                false
            );
        }
        return;
    }
    if (currentMenuIndexGD < 2 || currentMenuIndexGD > 6) {
        if (m_user.authenticated || currentMenuIndexGD == -1) {
            if (currentMenuIndexGD == 10) { // downloads
                //searchReq(fmt::format("{}/user/@me/objects?page={}&limit=false", HOST_URL, m_currentPage, m_currentPage), false);
                searchReq(fmt::format("{}/user/@me/downloads?page={}&limit={}", HOST_URL, m_currentPage, RESULT_LIMIT), false);
            } else if (currentMenuIndexGD == 1) { // favorited
                searchReq(fmt::format("{}/user/@me/favorites?page={}&limit={}", HOST_URL, m_currentPage, RESULT_LIMIT), false);
            } else if (currentMenuIndexGD == 7) { // pending 
                searchReq(fmt::format("{}/objects/pending?page={}", HOST_URL, m_currentPage), false);
            } else if (currentMenuIndexGD == 8) { // reports
                searchReq(fmt::format("{}/objects/reports?page={}", HOST_URL, m_currentPage), false);
            } else if (currentMenuIndexGD == -1) { // a user
                searchReq(fmt::format("{}/user/{}?page={}", HOST_URL, m_currentUserID, m_currentPage));
            }
        } else {
            FLAlertLayer::create("Error", "You aren't <cy>authenticated!</c>", "OK")->show();
            searchReq(fmt::format("{}/objects?page={}&category={}&limit={}", HOST_URL, m_currentPage, 0, RESULT_LIMIT));
        }
    } else {
        if (m_filterTags.empty()) {
            searchReq(fmt::format("{}/objects?page={}&category={}&featured={}&limit={}", HOST_URL, m_currentPage, Utils::intToCategory(currentMenuIndexGD), m_selectedFeatured ? "true" : "false", RESULT_LIMIT));
        } else {
            searchReq(
                fmt::format(
                    "{}/objects?page={}&category={}&tags={}&featured={}&limit={}",
                    HOST_URL,
                    m_currentPage,
                    Utils::intToCategory(currentMenuIndexGD),
                    Utils::url_encode(fmt::format("{}",fmt::join(m_filterTags, ","))),
                    m_selectedFeatured ? "true" : "false",
                    RESULT_LIMIT
                )
            );
        }
    }
}
void ObjectWorkshop::createCategoryBtn(const char* string, bool isSpriteFrame, int menuIndex, bool top) {
    auto bgNode = CategoryButton::create(string, isSpriteFrame, !top);
    auto btn = CCMenuItemSpriteExtra::create(
        bgNode,
        this,
        menu_selector(ObjectWorkshop::onSideButton)
    );
    btn->setID(fmt::format("category-{}"_spr, menuIndex));
    if (top) {
        m_categoryButtonsTop->addChild(btn);
        m_categoryButtonsTop->updateLayout();
    } else {
        m_categoryButtons->addChild(btn);
        m_categoryButtons->updateLayout();
    }
    
}

void ObjectWorkshop::onClickUser(int accountID) {
    if (auto oldItem = typeinfo_cast<CCMenuItemSpriteExtra*>(m_categoryButtonsTop->getChildByID(fmt::format("category-{}"_spr, currentMenuIndexGD)))) {
        static_cast<CategoryButton*>(oldItem->getChildren()->objectAtIndex(0))->setIndicatorState(false);
    }
    if (auto oldItem = typeinfo_cast<CCMenuItemSpriteExtra*>(m_categoryButtons->getChildByID(fmt::format("category-{}"_spr, currentMenuIndexGD)))) {
        static_cast<CategoryButton*>(oldItem->getChildren()->objectAtIndex(0))->setIndicatorState(false);
    }
    m_currentUserID = accountID;
    m_currentPage = 1;
    m_pageInput->setString("1");
    isSearching = false;
    currentMenuIndexGD = -1;
    RegenCategory();
}

void ObjectWorkshop::onAdminBtn(CCObject*) {
    AdminPopup::create(m_user, m_currentUser)->show();
}
void ObjectWorkshop::onClickObject(CCObject* sender) {
    auto menuItem = static_cast<CCMenuItemSpriteExtra*>(sender); // how could this go wrong
    if (auto objectItem = typeinfo_cast<ObjectItem*>(menuItem->getChildren()->objectAtIndex(0))) {
        auto objectData = objectItem->getData();
        ObjectPopup::create(objectData, m_user)->show();
    }
}

void ObjectWorkshop::onBackBtn(CCObject*) {
    if (m_currentMenu == 0) return;
    if (m_pageLabel != nullptr) m_pageLabel->setVisible(true);
    m_currentMenu = 0;
    rightBg->setVisible(true);
    obj_backBtn->setVisible(false);
    objectInfoNode->removeAllChildrenWithCleanup(true);
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
    if (!m_inEditor) {
        FLAlertLayer::create("Error", "You cannot <cy>upload objects</c> if you aren't in the <cl>Editor</c>!", "OK")->show();
        return;
    }
    m_filterTags.clear();
    m_currentMenu = 1;
    rightBg->setVisible(false);
    m_pageLabel->setVisible(false);
    obj_backBtn->setVisible(true);

    auto middleBg = CCScale9Sprite::create("square02_small.png");
    middleBg->setOpacity(25);
    middleBg->setContentSize({360, 238});
    objectInfoNode->addChildAtPosition(middleBg, Anchor::Center, {0, 5});

    auto previewBG = CCScale9Sprite::create("square02_small.png");
    previewBG->setOpacity(60);
    previewBG->setContentSize({ 360.F - 20.F, 82.F });
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
    m_objName->setCommonFilter(CommonFilter::Any);
    bottomBg->addChildAtPosition(m_objName, Anchor::Top, {0, -20});

    /*
    m_objDesc = TextInputNode::create("Description [Optional]", 300, {270, 60}, 90);//{270.F, 30.F}, 90);
    m_objDesc->getInput()->setScale(0.5F);
    bottomBg->addChildAtPosition(m_objDesc, Anchor::Center, {-1, -3});
    m_objDesc->addChildAtPosition(m_objDesc->getInput(), Anchor::Center);
    m_objDesc->setUpdateCallback([this](std::string text) {
        m_objDesc->getInput()->m_textArea->m_width = 300.0F / Utils::calculateScale(text, 50, 300, 1.0F, 0.5F);
        m_objDesc->getInput()->setScale(Utils::calculateScale(text, 50, 300, 0.75F, 0.45F));
        m_objDesc->getInput()->setPosition({
            Utils::calculateScale(text, 50, 300, 100, 60),
            Utils::calculateScale(text, 50, 300, 25, 20)
        });
    });
    //m_objDesc->getBackground()->setScale(0.5F);
    /\*m_objDesc->getBackground()->setContentSize({
        (m_objDesc->getSize().width - 20.F) * 2.F,
        (m_objDesc->getSize().height + 20.F) * 2.F
    });*\/
    /\*m_objDesc->getBackground()->setContentSize({
        520, 100
    });*/

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
            m_objDesc->getInputNode()->m_textLabel->setOpacity((p0.empty()) ? 255 : 0);
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

        if (m_oldCustomObjectButtonArray == nullptr) {
            m_oldCustomObjectButtonArray = editor->m_customObjectButtonArray;
        }
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
        if (currentMenuIndexGD < 2) return FLAlertLayer::create("Error", "You cannot search in <cy>My Objects</c> or <cy>Favorites</c>! Please select another category.", "OK")->show();
        m_currentPage = 1;
        m_pageInput->setString("1");
        isSearching = true;
        RegenCategory();
    } 
}

void ObjectWorkshop::onPendingBtn(CCObject*) {
    isSearching = false;
    m_currentPage = 1;
    m_pageInput->setString("1");
    currentMenuIndexGD = -1;
    RegenCategory();
}

void ObjectWorkshop::onUploadFilterBtn(CCObject*) {
    FiltersPopup::create(g_availableTags, m_filterTags, true, 0, true, [this](std::unordered_set<std::string> selectedTags, bool, bool, bool) {
        m_filterTags = selectedTags;
    })->show();
}
void ObjectWorkshop::onUpload(CCObject*) {
    if (auto editor = EditorUI::get()) {
        if (auto gameManager = GameManager::sharedState()) {
            if (m_filterTags.size() == 0) return FLAlertLayer::create("Error", "You must <cy>set a tag</c>! Click on the grey filter button to set one!", "OK")->show();
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
                obj.description = Utils::replaceAll(m_objDesc->getString(), "\\n", "\n");
            }
            obj.tags = m_filterTags;
            obj_backBtn->setVisible(false);
            objectInfoNode->removeAllChildrenWithCleanup(true);
            m_buttonMenu->removeChildByID("loadcomments"_spr);
            m_buttonMenu->removeChildByID("downloadbtn"_spr);
            m_buttonMenu->removeChildByID("loadmorebtn"_spr);
            m_buttonMenu->removeChildByID("uploadbtn"_spr);
            m_buttonMenu->removeChildByID("rulesbtn"_spr);
            m_buttonMenu->removeChildByID("tagbtn"_spr);
            loadingCircle = LoadingCircle::create();
            loadingCircle->setPosition({-20, -20});
            loadingCircle->setParentLayer(m_buttonMenu);
            loadingCircle->show();

            m_listener.cancel();
            m_buttonMenu->setEnabled(false);
            m_filterTags.clear();
            web::WebRequest req = web::WebRequest();
            req.userAgent(USER_AGENT);
            auto certValid = Mod::get()->getSettingValue<bool>("cert-valid");
            if (!certValid) {
                req.certVerification(certValid);
            }
            auto myjson = matjson::Value();
            myjson.set("token", m_token);
            myjson.set("name", obj.name);
            myjson.set("description", obj.description);
            myjson.set("tags", obj.tags);
            myjson.set("data", obj.objectString);
            req.header("Content-Type", "application/json");
            req.bodyJSON(myjson);
            m_listener.spawn(
                req.post(fmt::format("{}/objects/upload", HOST_URL)),
                [this](web::WebResponse value) {
                    m_currentMenu = 0;
                    rightBg->setVisible(true);
                    m_pageLabel->setVisible(true);
                    auto jsonRes = value.json().unwrapOrDefault();
                    if (Utils::notifError(jsonRes)) return;
                    Notification::create("Uploaded Object! It is now pending!", NotificationIcon::Success)->show();
                    if (loadingCircle != nullptr) loadingCircle->fadeAndRemove();
                    m_buttonMenu->setEnabled(true);
                }
            );
        }
    } else {
        FLAlertLayer::create("Error", "You cannot <cy>upload objects</c> if you aren't in the <cl>Editor</c>!", "OK")->show();
    }
}

// fix esc not working
void ObjectWorkshop::onClose(CCObject* sender) {
    m_listener.cancel();
    m_listener1.cancel();
    m_listener2.cancel();

    this->setKeypadEnabled(false);
    this->setTouchEnabled(false);
    if (!m_inEditor) {
        if (auto gameManager = GameManager::sharedState()) {
            if (gameManager->m_levelEditorLayer != nullptr) {
                gameManager->m_levelEditorLayer->release();
                gameManager->m_levelEditorLayer = nullptr; // i LOVE dangling pointres, anyways i cant do `delete` because apparently game crashes if i close
            }
            gameManager->m_editorEnabled = false;
            log::debug("delete LEL!");
        }
    }
    this->removeFromParentAndCleanup(true);
    if (auto editor = EditorUI::get()) {
        if (m_inEditor) {
            if (m_oldCustomObjectButtonArray != nullptr) {
                editor->m_customObjectButtonArray = m_oldCustomObjectButtonArray;
                editor->reloadCustomItems();
                editor->m_selectedObjectIndex = 0;
                editor->updateCreateMenu(false);
            }
        }
    }
    Popup::onClose(sender);
}

void ObjectWorkshop::keyDown(cocos2d::enumKeyCodes key, double timestamp) {
    if (key == cocos2d::enumKeyCodes::KEY_Escape) {
        if (rightBg->isVisible()) {
            this->onClose(nullptr);
        } else {
            this->onBackBtn(nullptr);
        }
        return;
    }
    if (key == cocos2d::enumKeyCodes::KEY_Space) return;
}

void ObjectWorkshop::keyBackClicked() {
    if (rightBg->isVisible()) {
        this->onClose(nullptr);
    } else {
        this->onBackBtn(nullptr);
    }
}

std::unordered_set<std::string> ObjectWorkshop::getTags() {
    return g_availableTags;
}
