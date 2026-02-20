#include "CasePopup.hpp"
#include "../../nodes/CaseCell.hpp"
#include "../../config.hpp"
#include "../../nodes/ScrollLayerExt.hpp"
#include "NewCasePopup.hpp"

bool CasePopup::init(UserData user, UserData managingUser) {
    if (!Popup::init(250.f, 200.f)) return false;
    m_user = user;
    m_managingUser = managingUser;
    this->setTitle("Cases (N/A)");
    Build<CCScale9Sprite>::create("square02_small.png").opacity(50).contentSize(205, 140).store(m_casesBG).parentAtPos(m_mainLayer, Anchor::Center, {0, -5});
    Build<CCSprite>::createSpriteName("GJ_updateBtn_001.png").scale(0.75f).intoMenuItem([this]() {
        onLoadCases();
    }).parentAtPos(m_buttonMenu, Anchor::BottomLeft, {3, 3});
    Build<CCSprite>::createSpriteName("GJ_plusBtn_001.png").scale(0.65f).intoMenuItem([this]() {
        if (!m_user.authenticated) return FLAlertLayer::create("Error", "how is this even possible!?", "OK")->show();
        NewCasePopup::create(m_managingUser, [this]() {
            onLoadCases();
        })->show();
    }).parentAtPos(m_buttonMenu, Anchor::BottomRight, {-3, 3});
    onLoadCases();
    this->setID("CasesPopup"_spr);
    return true;
}

void CasePopup::onClose(CCObject* sender) {
    m_listener.cancel();
    Popup::onClose(sender);
}

void CasePopup::onLoadCases() {
    m_casesBG->removeChildByID("commentscroll"_spr);
    m_mainLayer->removeChildByID("loadingCircle");
    auto loadingCircle = LoadingCircle::create();
    loadingCircle->setPositionX(-(m_mainLayer->getContentWidth() / 2));
    loadingCircle->setPositionY(-(m_mainLayer->getContentHeight() / 2) + 40);
    loadingCircle->setScale(0.7F);
    loadingCircle->setParentLayer(m_mainLayer);
    loadingCircle->show();
    loadingCircle->setID("loadingCircle");
    m_listener.cancel();
    web::WebRequest req = web::WebRequest();
    req.userAgent(USER_AGENT);

    auto token = Mod::get()->getSettingValue<std::string>("token");
    auto myjson = matjson::Value();
    myjson.set("token", token);
    req.header("Content-Type", "application/json");
    req.bodyJSON(myjson);
    m_listener.spawn(
        req.post(fmt::format("{}/user/{}/cases", HOST_URL, m_managingUser.account_id)),
        [this, loadingCircle](web::WebResponse value) {
            loadingCircle->fadeAndRemove();
            if (value.code() >= 500 && !value.ok()) {
                Notification::create("A server error occured. Check logs for info.", NotificationIcon::Error)->show();
                log::error("{}", value.string().unwrapOrDefault());
                return;
            }
            auto jsonRes = value.json().unwrapOrDefault();
            if (Utils::notifError(jsonRes)) return;
            auto arrayRes = jsonRes.get("results");
            auto c_total = jsonRes.get("total");
            std::vector<matjson::Value> array;
            if (arrayRes.isOk()) {
                array = arrayRes.unwrap().asArray().unwrap();
            }
            if (c_total.isOk()) {
                this->setTitle(fmt::format("Cases ({})", c_total.unwrap().asInt().unwrapOr(0)).c_str());
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
                auto caseData = matjson::Serialize<CaseData>::fromJson(item);
                if (caseData.isOk()) {
                    content->addChild(CaseCell::create(m_managingUser, caseData.unwrap(), [this]() {
                        onLoadCases();
                    }));
                } else {
                    log::error("Couldn't parse Case: {}", caseData.err());
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
        }
    );
}
