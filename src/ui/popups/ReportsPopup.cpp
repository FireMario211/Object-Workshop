#include "ReportsPopup.hpp"
#include "ObjectPopup.hpp"
#include "../../utils.hpp"

bool ReportsPopup::setup(std::vector<ReportData> reports, UserData user) {
    this->setTitle("Reports");
    auto bg = CCScale9Sprite::create("square02_small.png");
    bg->setOpacity(50);
    bg->setContentSize({205, 140});
    m_mainLayer->addChildAtPosition(bg, Anchor::Center, {0, -5});

    auto nodeArray = CCArray::create();
    auto scrollLayer = ScrollLayerExt::create({ 0, 0, 205.0F, 138.F }, true);
    auto content = CCMenu::create();
    content->setZOrder(2);
    content->setContentWidth(205.0F);
    scrollLayer->m_contentLayer->setContentSize({
        content->getContentSize().width,
        (reports.size() <= 3) ? 180.F : 45.F * reports.size()
    });
    content->setContentHeight(scrollLayer->m_contentLayer->getContentHeight());
    content->registerWithTouchDispatcher();
    
    scrollLayer->m_contentLayer->addChild(content);
    scrollLayer->setTouchEnabled(true);
    for (auto item : reports) {
        auto cell = cocos2d::extension::CCScale9Sprite::create("square02b_001.png");
        cell->setContentSize({ 200.0f, 40.0f });
        cell->setColor({95, 53, 31});

        auto menu = CCMenu::create();
        menu->setAnchorPoint({0.5, 0.5});
        menu->setContentSize(cell->getContentSize());

        auto authorName = CCLabelBMFont::create(std::to_string(item.accountID).c_str(), "bigFont.fnt");
        authorName->limitLabelWidth(80.0F, 0.45F, 0.1F); // 0.425
        auto authorBtn = CCMenuItemExt::createSpriteExtra(authorName, [this, item](CCObject*) {
            if (auto scene = CCScene::get()) {
                if (auto popup = typeinfo_cast<ObjectPopup*>(scene->getChildByID("ObjectPopup"_spr))) {
                    auto workshop = popup->getWorkshop();
                    if (workshop != nullptr) {
                        this->onClose(nullptr);
                        popup->onClose(nullptr);
                        workshop->onClickUser(item.accountID);
                    }
                } else {
                    log::error("Couldn't find ObjectPopup.");
                }
            }
        });
        authorBtn->setAnchorPoint({0, 0.5});
        authorBtn->setID("report/author"_spr);

        auto commentText = TextArea::create(
            item.reason, "chatFont.fnt",
            Utils::calculateScale(item.reason, 16, 100, 0.6F, 0.4F),
            150.F, { 0.0f, 0.5f }, // 1.0
            Utils::calculateScale(item.reason, 16, 100, 15.F, 7.F),
            true
        );
        commentText->setID("report/area"_spr);
        commentText->setAnchorPoint({ 0.0f, 0.5f });

        cell->addChildAtPosition(commentText, Anchor::Left, {4, 0});
        menu->addChildAtPosition(authorBtn, Anchor::TopLeft, {22, -9});

        cell->addChildAtPosition(menu, Anchor::Center);
        content->addChild(cell);
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
    if (reports.size() <= 3) {
        scrollLayer->setTouchEnabled(false);
        scrollLayer->setMouseEnabled(false);
    }
    content->setAnchorPoint({0.5, 0});
    content->setPosition({102.5F, -2});
    scrollLayer->setID("reportscroll"_spr);
    bg->addChildAtPosition(scrollLayer, Anchor::BottomLeft);
    scrollLayer->moveToTop();
    scrollLayer->fixTouchPrio();
    return true;
}
