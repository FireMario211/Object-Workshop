#include "Geode/utils/cocos.hpp"
#include "ReportsPopup.hpp"
#include "includes.h"
#include "ObjectPopup.hpp"
#include "CommentsPopup.hpp"
#include "../../config.hpp"
#include "../../utils.hpp"

std::string getStatus(ObjectData obj) {
    std::string status = "N/A";
    switch (obj.status) {
        case ObjectStatus::PENDING:
            status = "PENDING";
            break;
        case ObjectStatus::LISTED:
            status = "LISTED";
            break;
        case ObjectStatus::UNLISTED:
            status = "UNLISTED";
            break;
        case ObjectStatus::BANNED:
            status = "BANNED";
            break;
    }
    if (obj.featured == 1) {
        status += " (FEATURED)";
    }
    return status;
}

void ObjectPopup::onInfoBtn(CCObject*) {
    FLAlertLayer::create(
        "Object Info",
        fmt::format(
            "<cp>ID</c>: {}\n<cg>Uploader</c>: {}\n<cy>Uploaded</c>: {}\n<cy>Updated</c>: {}\n<cr>Objects</c>: {}\n<cl>Version</c>: {}\n<co>Tags</c>: {}\n<cb>Status</c>: {}",
            m_object.id,
            m_object.authorName,
            m_object.created,
            m_object.updated,
            std::count(m_object.objectString.begin(), m_object.objectString.end(), ';'),
            m_object.version,
            fmt::join(m_object.tags, ", "),
            getStatus(m_object)
        ).c_str(),
        "OK"
    )->show();
}

void ObjectPopup::onClose(CCObject* sender) {
    m_listener.getFilter().cancel();
    Popup::onClose(sender);
}

bool ObjectPopup::setup(ObjectData objectData, UserData user) {
    m_object = objectData;
    m_user = user;
    m_token = Mod::get()->getSettingValue<std::string>("token");
    if (auto scene = CCScene::get()) {
        if (auto workshop = typeinfo_cast<ObjectWorkshop*>(scene->getChildByID("objectworkshop"_spr))) {
            m_workshop = workshop;
            log::debug("Found {}! Setting m_workshop.", workshop->getID());
        } else {
            log::error("Couldn't find workshop.");
        }
    }
    auto infoBtn = CCMenuItemSpriteExtra::create(
        CCSprite::createWithSpriteFrameName("GJ_infoIcon_001.png"),
        this,
        menu_selector(ObjectPopup::onInfoBtn)
    );

    m_buttonMenu->addChildAtPosition(infoBtn, Anchor::TopRight, {-3, -3});

    // Top
    auto objTitleLabel = CCLabelBMFont::create(objectData.name.c_str(), "bigFont.fnt");
    auto objAuthorLabel = CCLabelBMFont::create(fmt::format("By {}", objectData.authorName).c_str(), "goldFont.fnt");
    objTitleLabel->limitLabelWidth(140.0F, 1.0F, 0.5F);
    objAuthorLabel->limitLabelWidth(140.0F, 1.0F, 0.5F);
    auto vLine1 = CCSprite::createWithSpriteFrameName("edit_vLine_001.png");
    vLine1->setRotation(90);
    vLine1->setScaleY(1.5F);
    auto objAuthorBtn = CCMenuItemSpriteExtra::create(objAuthorLabel, this, menu_selector(ObjectPopup::onAuthorBtn));
    m_mainLayer->addChildAtPosition(objTitleLabel, Anchor::Top, {0, -18});
    m_buttonMenu->addChildAtPosition(objAuthorBtn, Anchor::Top, {0, -38});
    m_mainLayer->addChildAtPosition(vLine1, Anchor::Top, {0, -52});

    // Middle
    if (m_workshop != nullptr) {
        auto editorLayer = m_workshop->m_editorLayer;
        m_previewBG = ExtPreviewBG::create(editorLayer, objectData.objectString, {240.F, 82.F});
        m_mainLayer->addChildAtPosition(m_previewBG, Anchor::Center, {0, 40});
    }

    auto resetZoomSpr = CCSprite::createWithSpriteFrameName("GJ_updateBtn_001.png");
    resetZoomSpr->setScale(0.4F);
    auto resetZoomBtn = CCMenuItemSpriteExtra::create(
        resetZoomSpr, this, menu_selector(ObjectPopup::onResetZoom)
    );
    m_buttonMenu->addChildAtPosition(resetZoomBtn, Anchor::Right, {-32, 78});


    auto leftSideBG1 = CCScale9Sprite::create("square02_small.png");
    leftSideBG1->setOpacity(50);
    leftSideBG1->setScale(0.8F);
    leftSideBG1->setContentSize({ 27.F, 102.F });
    m_mainLayer->addChildAtPosition(leftSideBG1, Anchor::Left, {17, 40 });

    auto leftMenu = CCMenu::create();
    leftMenu->setContentSize({27, 102});

    auto trashSpr = CCSprite::createWithSpriteFrameName("GJ_trashBtn_001.png");
    trashSpr->setScale(0.6F);
    auto trashBtn = CCMenuItemSpriteExtra::create(
        trashSpr, this, menu_selector(ObjectPopup::onTrashBtn)
    );
    auto editSpr = CCSprite::createWithSpriteFrameName("GJ_editBtn_001.png");
    editSpr->setScale(0.3F);
    auto editBtn = CCMenuItemSpriteExtra::create(
        editSpr, this, menu_selector(ObjectPopup::onEditBtn)
    );
    auto reviewSpr = CCSprite::createWithSpriteFrameName("GJ_starBtnMod_001.png");
    reviewSpr->setScale(0.5F);
    auto reviewBtn = CCMenuItemSpriteExtra::create(
        reviewSpr, this, menu_selector(ObjectPopup::onReviewBtn)
    );
    auto reportSpr = CCSprite::createWithSpriteFrameName("GJ_reportBtn_001.png");
    reportSpr->setScale(0.5F);
    auto reportBtn = CCMenuItemSpriteExtra::create(
        reportSpr, this, menu_selector(ObjectPopup::onReportBtn)
    );
    if (m_user.account_id == objectData.authorAccId || m_user.role >= 2) {
        leftMenu->addChild(editBtn);
    }
    if (m_user.account_id == objectData.authorAccId || m_user.role == 3) {
        leftMenu->addChild(trashBtn);
    }
    if (m_user.role >= 2 && objectData.status == 0) {
        leftMenu->addChild(reviewBtn);
    }
    if (m_user.role >= 3) {
        auto featureSpr = CCSprite::createWithSpriteFrameName("GJ_starBtnMod_001.png");
        featureSpr->setScale(0.5F);
        featureSpr->setColor({0,255,0});
        auto featureBtn = CCMenuItemSpriteExtra::create(
            featureSpr, this, menu_selector(ObjectPopup::onFeatureBtn)
        );
        leftMenu->addChild(featureBtn);

        if (!m_object.reports.empty()) {
            auto admreportSpr = CCSprite::createWithSpriteFrameName("GJ_reportBtn_001.png");
            admreportSpr->setScale(0.5F);
            admreportSpr->setColor({0, 255, 0});
            auto admreportBtn = CCMenuItemExt::createSpriteExtra(admreportSpr, [this](CCObject*) {
                ReportsPopup::create(m_object.reports, {})->show();
            });
            leftMenu->addChild(admreportBtn);
        }
    }
    if (m_user.account_id != objectData.authorAccId && m_user.account_id > 0) {
        leftMenu->addChild(reportBtn);
    }

    leftSideBG1->addChildAtPosition(leftMenu, Anchor::Center);

    leftMenu->setLayout(
        RowLayout::create()
            ->setAxisAlignment(AxisAlignment::Even)
            ->setCrossAxisAlignment(AxisAlignment::End)
            ->setAutoScale(true)
            ->setCrossAxisOverflow(false)
            ->setDefaultScaleLimits(.1f, 1.f)
            ->setGap(3)
            ->setGrowCrossAxis(true)
    );
    leftMenu->updateLayout();

    auto rightSideBG1 = CCScale9Sprite::create("square02_small.png");
    rightSideBG1->setOpacity(50);
    rightSideBG1->setScale(0.8F);
    rightSideBG1->setContentSize({ 27.F, 102.F });
    m_mainLayer->addChildAtPosition(rightSideBG1, Anchor::Right, {-17, 40 });
    m_slider = Slider::create(this, menu_selector(ObjectPopup::onSliderZoom), 1.0f);
    m_slider->setValue((m_previewBG->getScale() / (float)MAX_ZOOM));
    m_oldSliderValue = m_slider->getValue();
    m_slider->setBarVisibility(false);
    m_slider->setRotated(true);
    m_slider->hideGroove(true);
    m_slider->setMaxOffset(75.F);

    m_slider->setContentSize({0, 0});
    rightSideBG1->addChildAtPosition(m_slider, Anchor::Center);

    // Middle Bottom

    auto leftSideBG2 = CCScale9Sprite::create("square02_small.png");
    leftSideBG2->setOpacity(50);
    leftSideBG2->setContentSize({123, 40});
    leftSideBG2->setAnchorPoint({1, 0.5});
    m_mainLayer->addChildAtPosition(leftSideBG2, Anchor::Right, {-7, -24});
    
    auto tagsLabel = CCLabelBMFont::create("Tags:", "goldFont.fnt");
    auto tagsNode = FiltersPopup::createTags(objectData.tags, {125, 25}, {0.5, 0.5}, AxisAlignment::Center);
    tagsLabel->setScale(0.41F);
    tagsLabel->setAnchorPoint({0.5, 1});
    leftSideBG2->addChildAtPosition(tagsLabel, Anchor::Top, {0, 0});
    leftSideBG2->addChildAtPosition(tagsNode, Anchor::Center, {0, -5});
    
    // Bottom

    auto textAreaDesc = MDTextArea::create(objectData.description, {200, 112});
    textAreaDesc->setScale(0.8F);
    textAreaDesc->setAnchorPoint({0, 0.5});
    m_mainLayer->addChildAtPosition(textAreaDesc, Anchor::Left, {12, -48});

    auto leftSideBG = CCScale9Sprite::create("square02_small.png");
    leftSideBG->setOpacity(50);
    leftSideBG->setContentSize({160, 33});
    leftSideBG->setAnchorPoint({0, 0.5});
    auto infoLabels = CCLabelBMFont::create(
        fmt::format(
            "Objects: {}\nVersion: {}\nStatus: {}",
            std::count(objectData.objectString.begin(), objectData.objectString.end(), ';'),
            objectData.version,
            getStatus(objectData)
        ).c_str(),
        "bigFont.fnt"
    );
    infoLabels->setContentHeight(100);
    infoLabels->setAnchorPoint({0, 1.0});
    infoLabels->setScale(0.31F);
    leftSideBG->addChildAtPosition(infoLabels, Anchor::TopLeft, {3, -1});
    m_mainLayer->addChildAtPosition(leftSideBG, Anchor::BottomLeft, {6, 25});

    auto rightSideBG3 = CCScale9Sprite::create("square02_small.png");
    rightSideBG3->setOpacity(50);
    rightSideBG3->setContentSize({144, 55});
    rightSideBG3->setScale(0.85F);
    rightSideBG3->setAnchorPoint({1, 0.5});
    m_mainLayer->addChildAtPosition(rightSideBG3, Anchor::Right, {-7, -70});

    auto rateItLbl = CCLabelBMFont::create("Rate It!", "bigFont.fnt");
    rateItLbl->setScale(0.35F);
    rightSideBG3->addChildAtPosition(rateItLbl, Anchor::Top, {0, -10});

    auto starsNode = ObjectItem::createClickableStars(this, menu_selector(ObjectPopup::onRateBtn));
    starsNode->setLayout(
        RowLayout::create()
            ->setAxisAlignment(AxisAlignment::Center)
            ->setAutoScale(false)
            ->setCrossAxisOverflow(false)
            ->setGap(4)
            ->setGrowCrossAxis(true)
    );
    starsNode->updateLayout();
    starsNode->setScale(1.55F);
    rightSideBG3->addChildAtPosition(starsNode, Anchor::Top, {0, -32});

    std::ostringstream oss;
    oss << std::fixed << std::setprecision(2) << objectData.rating;
    auto ratingLbl = CCLabelBMFont::create(fmt::format("{} ({})", oss.str(), objectData.ratingCount).c_str(), "bigFont.fnt");

    for (int i = 0; i < 4; i++) {
        // why doesnt CCFontSprite EXIST!?
        auto fontSpr = static_cast<CCSprite*>(ratingLbl->getChildren()->objectAtIndex(i));
        fontSpr->setColor(ObjectItem::starColor(objectData.rating));
    }

    ratingLbl->setScale(0.3F);
    rightSideBG3->addChildAtPosition(ratingLbl, Anchor::Bottom, {3, 9});

    auto vLine2 = CCSprite::createWithSpriteFrameName("edit_vLine_001.png");
    vLine2->setRotation(90);
    rightSideBG3->addChildAtPosition(vLine2, Anchor::Top, {0, -20});
    
    auto bottomRightBG = CCScale9Sprite::create("square02_small.png");
    bottomRightBG->setOpacity(50);
    bottomRightBG->setScale(0.85F);
    bottomRightBG->setContentSize({144, 39});
    bottomRightBG->setAnchorPoint({1, 0.5});
    m_mainLayer->addChildAtPosition(bottomRightBG, Anchor::BottomRight, {-7, 25});
    auto downloadSpr = CCSprite::createWithSpriteFrameName("GJ_downloadBtn_001.png");
    downloadSpr->setScale(0.55F);
    auto unfavSpr = CCSprite::createWithSpriteFrameName("gj_heartOff_001.png");
    unfavSpr->setScale(0.75F);
    auto favSpr = CCSprite::createWithSpriteFrameName("gj_heartOn_001.png");
    favSpr->setScale(0.75F);

    auto downloadBtn = CCMenuItemSpriteExtra::create(downloadSpr, this, menu_selector(ObjectPopup::onDownloadBtn));

    auto favBtn = CCMenuItemToggler::create(unfavSpr, favSpr, this, menu_selector(ObjectPopup::onFavBtn));
    if (m_workshop != nullptr) {
        favBtn->toggle(Utils::arrayIncludes(m_workshop->m_user.favorites, objectData.id));
    }
    m_buttonMenu->addChildAtPosition(downloadBtn, Anchor::BottomRight, {-90, 29});
    m_buttonMenu->addChildAtPosition(favBtn, Anchor::BottomRight, {-48, 29});

    downloadsLabel = CCLabelBMFont::create(std::to_string(objectData.downloads).c_str(), "bigFont.fnt");
    favoritesLabel = CCLabelBMFont::create(std::to_string(objectData.favorites).c_str(), "bigFont.fnt");
    downloadsLabel->setAnchorPoint({0.5, 0});
    favoritesLabel->setAnchorPoint({0.5, 0});
    downloadsLabel->limitLabelWidth(50.0F, 0.32F, 0.1F);
    favoritesLabel->limitLabelWidth(50.0F, 0.32F, 0.1F);

    bottomRightBG->addChildAtPosition(downloadsLabel, Anchor::Bottom, {-25,1});
    bottomRightBG->addChildAtPosition(favoritesLabel, Anchor::Bottom, {25,1});

    auto commentSpr = CCSprite::createWithSpriteFrameName("GJ_chatBtn_001.png");
    commentSpr->setScale(0.65F);

    auto commentBtn = CCMenuItemSpriteExtra::create(commentSpr, this, menu_selector(ObjectPopup::onCommentsBtn));
    m_buttonMenu->addChildAtPosition(commentBtn, Anchor::BottomRight, {-3, 3});
    this->setID("ObjectPopup"_spr);
    return true;
}

void ObjectPopup::onSliderZoom(CCObject*) {
    float value = m_slider->getValue();
    if (m_previewBG != nullptr) m_previewBG->setZoom(value * (float)MAX_ZOOM);
}
void ObjectPopup::onResetZoom(CCObject*) {
    m_slider->setValue(m_oldSliderValue);
    if (m_previewBG != nullptr) m_previewBG->resetZoom();
}

void ObjectPopup::onTrashBtn(CCObject*) {
    geode::createQuickPopup(
        "Warning",
        "Are you sure you want to <cy>delete this object</c>?\nYou <cr>cannot go back from this</c>!",
        "No",
        "Yes",
        [this](auto, bool btn2) {
            if (btn2) {
                m_listener.getFilter().cancel();
                m_listener.bind([this] (web::WebTask::Event* e) {
                    if (web::WebResponse* value = e->getValue()) {
                        auto jsonRes = value->json().unwrapOrDefault();
                        if (Utils::notifError(jsonRes)) return;
                        auto message = jsonRes.get("message");
                        if (message.isOk()) {
                            Notification::create(message.unwrap().asString().unwrapOrDefault(), NotificationIcon::Success)->show();
                        } else {
                            log::error("Unknown response, expected message. {}", message.err());
                            Notification::create("Got an unknown response, check logs for details.", NotificationIcon::Warning)->show();
                        }
                        if (m_workshop != nullptr) {
                            this->onClose(nullptr);
                            m_workshop->RegenCategory();
                        }
                        return;
                    } else if (web::WebProgress* progress = e->getProgress()) {
                        // The request is still in progress...
                    } else if (e->isCancelled()) {
                        log::error("Request was cancelled.");
                    }
                });
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
                m_listener.setFilter(req.post(fmt::format("{}/objects/{}/delete", HOST_URL, m_object.id)));
            }
        },
        true,
        true
    );
}

void ObjectPopup::onEditBtn(CCObject*) {
    if (m_workshop != nullptr) {
        EditPopup::create(m_object, m_workshop->getTags(), m_user)->show();
    }
}
void ObjectPopup::onReviewBtn(CCObject*) {
    auto popup = VotePopup::create("Review", [this](bool review) {
        m_listener.getFilter().cancel();
        m_listener.bind([this] (web::WebTask::Event* e) {
            if (web::WebResponse* value = e->getValue()) {
                auto jsonRes = value->json().unwrapOrDefault();
                if (Utils::notifError(jsonRes)) return;
                auto message = jsonRes.get("message");
                if (message.isOk()) {
                    Notification::create(message.unwrap().asString().unwrapOrDefault(), NotificationIcon::Success)->show();
                } else {
                    log::error("Unknown response, expected message. {}", message.err());
                    Notification::create("Got an unknown response, check logs for details.", NotificationIcon::Warning)->show();
                }
                if (m_workshop != nullptr) {
                    this->onClose(nullptr);
                    m_workshop->RegenCategory();
                }
                return;
            } else if (web::WebProgress* progress = e->getProgress()) {
                // The request is still in progress...
            } else if (e->isCancelled()) {
                log::error("Request was cancelled.");
            }
        });
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
        m_listener.setFilter(req.post(fmt::format("{}/objects/{}/{}", HOST_URL, m_object.id, (review) ? "accept" : "reject")));
    });
    popup->setWarning("Are you sure you want to <cy>accept this object</c>?", "Are you sure you want to <cy>reject this object</c>?");
    popup->show();
}

void ObjectPopup::onFeatureBtn(CCObject*) {
    geode::createQuickPopup(
    "Warning",
    fmt::format(
        "Are you sure you want to {}?",
        (m_object.featured == 0) ? "<cy>feature this object</c>" : "<cr>unfeature this object</c>"
    ),
    "No",
    "Yes",
    [this](auto, bool btn2) {
        if (btn2) {
            m_listener.getFilter().cancel();
            m_listener.bind([this] (web::WebTask::Event* e) {
                if (web::WebResponse* value = e->getValue()) {
                    auto jsonRes = value->json().unwrapOrDefault();
                    if (Utils::notifError(jsonRes)) return;
                    auto message = jsonRes.get("message");
                    if (message.isOk()) {
                        Notification::create(message.unwrap().asString().unwrapOrDefault(), NotificationIcon::Success)->show();
                    } else {
                        log::error("Unknown response, expected message. {}", message.err());
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
            req.userAgent(USER_AGENT);
            auto certValid = Mod::get()->getSettingValue<bool>("cert-valid");
            if (!certValid) {
                req.certVerification(certValid);
            }
            auto myjson = matjson::Value();
            myjson.set("token", m_token);
            req.header("Content-Type", "application/json");
            req.bodyJSON(myjson);
            m_listener.setFilter(req.post(fmt::format("{}/objects/{}/feature", HOST_URL, m_object.id)));
        }
    });
}

void ObjectPopup::onReportBtn(CCObject*) {
    ReportPopup::create(m_object)->show();
}

void ObjectPopup::onRateBtn(CCObject* sender) {
    if (!m_user.authenticated) return FLAlertLayer::create("Error", "You cannot rate objects as you are <cy>not authenticated!</c>", "OK")->show();
    m_listener.getFilter().cancel();
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
    m_listener.bind([this] (web::WebTask::Event* e) {
        if (web::WebResponse* value = e->getValue()) {
            auto jsonRes = value->json().unwrapOrDefault();
            if (Utils::notifError(jsonRes)) return;
            Notification::create("Rated!", NotificationIcon::Success)->show();
            return;
        } else if (web::WebProgress* progress = e->getProgress()) {
            // The request is still in progress...
        } else if (e->isCancelled()) {
            log::error("Request was cancelled.");
        }
    });
    web::WebRequest req = web::WebRequest();
    req.userAgent(USER_AGENT);
    auto certValid = Mod::get()->getSettingValue<bool>("cert-valid");
    if (!certValid) {
        req.certVerification(certValid);
    }
    auto myjson = matjson::Value();
    myjson.set("token", m_token);
    myjson.set("stars", std::stoi(menuItem->getID()));
    req.header("Content-Type", "application/json");
    req.bodyJSON(myjson);
    m_listener.setFilter(req.post(fmt::format("{}/objects/{}/rate", HOST_URL, m_object.id)));
}

void ObjectPopup::actuallyDownload() {
    if (auto gameManager = GameManager::sharedState()) {
        if (auto editorUI = CustomObjects::get()) {
            gameManager->addNewCustomObject(m_object.objectString);
            editorUI->reloadCustomItems();
            Notification::create("Downloaded object!", NotificationIcon::Success)->show();
        }
    }
}

void ObjectPopup::onDownloadBtn(CCObject*) {
    if (m_workshop != nullptr) {
        if (!m_workshop->m_inEditor) {
            FLAlertLayer::create("Error", "You need to be in the <cl>Editor</c> to <cg>download objects</c>!", "OK")->show();
            return;
        }
    }
    if (Utils::arrayIncludes(m_user.downloaded, m_object.id)) {
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
        if (m_object.status == ObjectStatus::PENDING) return actuallyDownload();
        m_listener.getFilter().cancel();
        m_listener.bind([this] (web::WebTask::Event* e) {
            if (web::WebResponse* value = e->getValue()) {
                if (value->json().isErr()) return log::error("Response is not JSON.");
                auto jsonRes = value->json().unwrapOrDefault();
                if (Utils::notifError(jsonRes)) return;
                if (downloadsLabel != nullptr) downloadsLabel->setString(std::to_string(m_object.downloads).c_str());
                return;
            } else if (web::WebProgress* progress = e->getProgress()) {
                // The request is still in progress...
            } else if (e->isCancelled()) {
                log::error("Request was cancelled.");
            }
        });
        m_object.downloads++;
        if (m_workshop != nullptr) {
            m_workshop->m_user.downloaded.push_back(m_object.id);
        }
        actuallyDownload();
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
        m_listener.setFilter(req.post(fmt::format("{}/objects/{}/download", HOST_URL, m_object.id)));
    }
}

void ObjectPopup::onFavBtn(CCObject*) {
    if (!m_user.authenticated) return FLAlertLayer::create("Error", "You cannot favorite levels as you are <cy>not authenticated!</c>", "OK")->show();
    m_listener.getFilter().cancel();
    m_listener.bind([this] (web::WebTask::Event* e) {
        if (web::WebResponse* value = e->getValue()) {
            auto jsonRes = value->json().unwrapOrDefault();
            if (Utils::notifError(jsonRes)) return;
            auto message = jsonRes.get("message");
            if (message.isOk()) {
                Notification::create(message.unwrap().asString().unwrapOrDefault(), NotificationIcon::Success)->show();
            } else {
                log::error("Unknown response, expected message. {}", message.err());
                Notification::create("Got an unknown response, check logs for details.", NotificationIcon::Warning)->show();
            }
            m_object.favorited = !m_object.favorited;
            if (m_workshop != nullptr) {
                if (m_object.favorited) {
                    m_object.favorites++;
                    m_user.favorites.push_back(m_object.id);
                } else {
                    m_object.favorites--;
                    auto it = std::find(m_user.favorites.begin(), m_user.favorites.end(), m_object.id);
                    if (it != m_user.favorites.end()) { 
                        m_user.favorites.erase(it); 
                    }
                }
            }
            if (favoritesLabel != nullptr) favoritesLabel->setString(std::to_string(m_object.favorites).c_str());
            return;
        } else if (web::WebProgress* progress = e->getProgress()) {
            // The request is still in progress...
        } else if (e->isCancelled()) {
            log::error("Request was cancelled.");
        }
    });
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
    m_listener.setFilter(req.post(fmt::format("{}/objects/{}/favorite", HOST_URL, m_object.id)));
}

void ObjectPopup::onCommentsBtn(CCObject*) {
    CommentsPopup::create(m_object, m_user)->show();
}

void ObjectPopup::onAuthorBtn(CCObject*) {
    if (m_workshop != nullptr) {
        m_workshop->onClickUser(m_object.authorAccId);
        this->onClose(nullptr);
    }
}
