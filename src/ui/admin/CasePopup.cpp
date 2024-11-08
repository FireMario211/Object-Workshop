#include "CasePopup.hpp"
#include "../../nodes/CaseCell.hpp"
#include "../../config.hpp"
#include "../../nodes/ScrollLayerExt.hpp"
#include "NewCasePopup.hpp"

bool CasePopup::setup(UserData user, UserData managingUser) {
    m_user = user;
    m_managingUser = managingUser;
    this->setTitle("Cases (N/A)");
    m_casesBG = CCScale9Sprite::create("square02_small.png");
    m_casesBG->setOpacity(50);
    m_casesBG->setContentSize({205, 140});
    m_mainLayer->addChildAtPosition(m_casesBG, Anchor::Center, {0, -5});

    auto refreshSpr = CCSprite::createWithSpriteFrameName("GJ_updateBtn_001.png");
    refreshSpr->setScale(0.75F);
    auto refreshBtn = CCMenuItemSpriteExtra::create(refreshSpr, this, menu_selector(CasePopup::onLoadCases));
    m_buttonMenu->addChildAtPosition(refreshBtn, Anchor::BottomLeft, {3, 3});

    auto newCaseBtn = CCMenuItemExt::createSpriteExtraWithFrameName("GJ_plusBtn_001.png", 0.65F, [this](CCObject*) {
        if (!m_user.authenticated) return FLAlertLayer::create("Error", "how is this even possible!?", "OK")->show();
        NewCasePopup::create(m_managingUser, [this]() {
            onLoadCases(nullptr);
        })->show();
    });
    m_buttonMenu->addChildAtPosition(newCaseBtn, Anchor::BottomRight, {-3, 3});

    onLoadCases(nullptr);
    this->setID("CasesPopup"_spr);
    return true;
}

void CasePopup::onClose(CCObject* sender) {
    m_listener.getFilter().cancel();
    Popup::onClose(sender);
}

void CasePopup::onLoadCases(CCObject*) {
    m_casesBG->removeChildByID("commentscroll"_spr);
    m_mainLayer->removeChildByID("loadingCircle");
    auto loadingCircle = LoadingCircle::create();
    loadingCircle->setPositionX(-(m_mainLayer->getContentWidth() / 2));
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
            if (isError) return Notification::create(isError->c_str(), NotificationIcon::Error)->show();
            auto arrayRes = jsonRes.try_get<matjson::Array>("results");
            auto c_total = jsonRes.try_get<int>("total");
            matjson::Array array;
            if (arrayRes) {
                array = arrayRes.value();
            }
            if (c_total) {
                this->setTitle(fmt::format("Cases ({})", c_total.value()).c_str());
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
                auto c_id = item.try_get<int>("case_id");
                auto c_case_type = item.try_get<int>("case_type");
                auto c_account_id = item.try_get<int>("account_id");
                auto c_staff_account_id = item.try_get<int>("staff_account_id");
                auto c_reason = item.try_get<std::string>("reason");
                auto c_timestamp = item.try_get<std::string>("timestamp");
                auto c_expiration = item.try_get<std::string>("timestamp");
                auto c_ack = item.try_get<bool>("ack");
                auto c_ack_timestamp = item.try_get<std::string>("ack_timestamp");
                auto c_staff_account_name = item.try_get<std::string>("staff_account_name");
                if (
                    c_id && c_case_type &&
                    c_account_id && c_staff_account_id && c_staff_account_name &&
                    c_reason && c_ack &&
                    c_timestamp && c_ack_timestamp && c_expiration
                ) {
                    content->addChild(CaseCell::create(m_managingUser, {
                        c_id.value(),
                        0,
                        static_cast<CaseType>(c_case_type.value()),
                        c_reason.value(),
                        c_timestamp.value(),
                        c_account_id.value(),
                        c_staff_account_id.value(),
                        c_expiration.value(),
                        c_ack.value(),
                        c_ack_timestamp.value(),
                        c_staff_account_name.value()
                    }, [this]() {
                        onLoadCases(nullptr);
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
            m_casesBG->addChildAtPosition(scrollLayer, Anchor::BottomLeft);
            scrollLayer->moveToTop();
            scrollLayer->fixTouchPrio();
        } else if (web::WebProgress* progress = e->getProgress()) {
            // The request is still in progress...
        } else if (e->isCancelled()) {
            log::error("Request was cancelled.");
        }
    });
    web::WebRequest req = web::WebRequest();
    req.userAgent(USER_AGENT);

    auto token = Mod::get()->getSettingValue<std::string>("token");
    auto myjson = matjson::Value();
    myjson.set("token", token);
    req.header("Content-Type", "application/json");
    req.bodyJSON(myjson);
    m_listener.setFilter(req.post(fmt::format("{}/user/{}/cases", HOST_URL, m_managingUser.account_id)));
}
