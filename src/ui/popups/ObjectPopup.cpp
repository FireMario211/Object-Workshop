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
        case ObjectStatus::PENDING_DELETION:
            status = "PENDING DELETION";
            break;
    }
    if (obj.appealed) {
        status += " (APPEALED)";
    }
    if (obj.featured == 1) {
        status += " (FEATURED)";
    }
    return status;
}

bool ObjectPopup::init(ObjectData objectData, UserData user) {
    if (!Popup::init(300.f, 275.f)) return false;
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
    Build<CCSprite>::createSpriteName("GJ_infoIcon_001.png").intoMenuItem([this]() {
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
    }).parentAtPos(m_buttonMenu, Anchor::TopRight, {-3, -3});

    // Top
    Build<CCLabelBMFont>::create(objectData.name.c_str(), "bigFont.fnt").limitLabelWidth(140.f, 1.0f, 0.5f).parentAtPos(m_mainLayer, Anchor::Top, {0, -18});
    Build<CCLabelBMFont>::create(fmt::format("By {}", objectData.authorName).c_str(), "goldFont.fnt").limitLabelWidth(140.f, 1.0f, 0.5f).intoMenuItem([this]() {
        if (m_workshop != nullptr) {
            m_workshop->onClickUser(m_object.authorAccId);
            this->onClose(nullptr);
        }
    }).parentAtPos(m_buttonMenu, Anchor::Top, {0, -38});
    Build<CCSprite>::createSpriteName("edit_vLine_001.png").rotation(90).scale(1.5f).parentAtPos(m_mainLayer, Anchor::Top, {0, -52});

    // Middle
    if (m_workshop != nullptr) {
        auto editorLayer = m_workshop->m_editorLayer;
        m_previewBG = ExtPreviewBG::create(editorLayer, objectData.objectString, {240.F, 82.F});
        m_mainLayer->addChildAtPosition(m_previewBG, Anchor::Center, {0, 40});
    }

    Build<CCSprite>::createSpriteName("GJ_updateBtn_001.png").scale(0.4f).intoMenuItem([this]() {
        m_slider->setValue(m_oldSliderValue);
        if (m_previewBG != nullptr) m_previewBG->resetZoom();
    }).parentAtPos(m_buttonMenu, Anchor::Right, {-32, 78});

    //m_user.role = 2;
    // leftSideBG1
    Build<CCScale9Sprite>::create("square02_small.png").contentSize(27,102).scale(0.8f).opacity(50).with([this, objectData](auto node) {
        Build<CCMenu>::create().contentSize(27, 102).layout(RowLayout::create()
            ->setAxisAlignment(AxisAlignment::Even)
            ->setCrossAxisAlignment(AxisAlignment::End)
            ->setAutoScale(true)
            ->setCrossAxisOverflow(false)
            ->setDefaultScaleLimits(.1f, 1.f)
            ->setGap(3)
            ->setGrowCrossAxis(true)
        ).with([this, objectData](auto menu) {
            if (m_user.account_id == objectData.authorAccId || m_user.role >= 2) {
                Build<CCSprite>::createSpriteName("GJ_editBtn_001.png").scale(0.3f).intoMenuItem([this]() {
                    if (m_workshop != nullptr) {
                        EditPopup::create(m_object, m_workshop->getTags(), m_user)->show();
                    }
                }).parent(menu);
            }
            if (m_user.account_id == objectData.authorAccId || m_user.role == 3) {
                Build<CCSprite>::createSpriteName("GJ_trashBtn_001.png").scale(0.6f).intoMenuItem([this]() {
                    geode::createQuickPopup(
                        "Warning",
                        "Are you sure you want to <cy>delete this object</c>?\nYou <cr>cannot go back from this</c>!",
                        "No",
                        "Yes",
                        [this](auto, bool btn2) {
                            if (btn2) {
                                sendRequest(fmt::format("{}/objects/{}/delete", HOST_URL, m_object.id));
                            }
                        },
                        true,
                        true
                    );
                }).parent(menu);
            }
            if (m_user.account_id != objectData.authorAccId && m_user.account_id > 0) {
                Build<CCSprite>::createSpriteName("GJ_reportBtn_001.png").scale(0.5f).intoMenuItem([this]() {
                    ReportPopup::create(m_object, ReportActionType::Report)->show();
                }).parent(menu);
            }
            if (m_user.role >= 3) {
                if (!m_object.reports.empty()) {
                    Build<CCSprite>::createSpriteName("GJ_reportBtn_001.png").scale(0.5f).color(0,255,0).intoMenuItem([this]() {
                        ReportsPopup::create(m_object.reports, {})->show();
                    }).parent(menu);
                }
            }
            if (m_user.account_id == objectData.authorAccId) {
                if (m_object.status == ObjectStatus::PENDING_DELETION) {
                    Build<CCSprite>::createSpriteName("GJ_reportBtn_001.png").scale(0.5f).color(0,255,0).intoMenuItem([this]() {
                        ReportPopup::create(m_object, ReportActionType::Appeal)->show();
                    }).parent(menu);
                }
            }
        }).parentAtPos(node, Anchor::Center).updateLayout();
    }).parentAtPos(m_mainLayer, Anchor::Left, {17, 40});

    // rightSideBG1
    Build<CCScale9Sprite>::create("square02_small.png").opacity(50).scale(0.8f).contentSize(27,102).with([this](auto node) {
        m_slider = Slider::create(this, menu_selector(ObjectPopup::onSliderZoom), 1.0f);
        m_slider->setValue((m_previewBG->getScale() / (float)MAX_ZOOM));
        m_oldSliderValue = m_slider->getValue();
        m_slider->setBarVisibility(false);
        m_slider->setRotated(true);
        m_slider->hideGroove(true);
        m_slider->setMaxOffset(75.F);
        m_slider->setContentSize({0, 0});
        node->addChildAtPosition(m_slider, Anchor::Center);
    }).parentAtPos(m_mainLayer, Anchor::Right, {-17, 40});

    // Middle Bottom

    // leftSideBG2
    Build<CCScale9Sprite>::create("square02_small.png").opacity(50).contentSize(123,40).anchorPoint(1,0.5f).with([objectData](auto node) {
        Build<CCLabelBMFont>::create("Tags:", "goldFont.fnt").scale(0.41f).anchorPoint(0.5, 1).parentAtPos(node, Anchor::Top, {0, 0});
        node->addChildAtPosition(FiltersPopup::createTags(objectData.tags, {125, 25}, {0.5, 0.5}, AxisAlignment::Center), Anchor::Center, {0, -5});
    }).parentAtPos(m_mainLayer, Anchor::Right, {-7, -24});

    // Bottom
    auto textAreaDesc = MDTextArea::create(objectData.description, {200, 112});
    textAreaDesc->setScale(0.8F);
    textAreaDesc->setAnchorPoint({0, 0.5});
    m_mainLayer->addChildAtPosition(textAreaDesc, Anchor::Left, {12, -48});

    // leftSideBG
    Build<CCScale9Sprite>::create("square02_small.png").opacity(50).contentSize(160,33).anchorPoint(0,0.5).with([objectData](auto node) {
        Build<CCLabelBMFont>::create(
            fmt::format(
                "Objects: {}\nVersion: {}\nStatus: {}",
                std::count(objectData.objectString.begin(), objectData.objectString.end(), ';'),
                objectData.version,
                getStatus(objectData)
            ).c_str(),
            "bigFont.fnt"
        ).height(100).anchorPoint(0, 1).scale(0.31f).parentAtPos(node, Anchor::TopLeft, {3, -1});
    }).parentAtPos(m_mainLayer, Anchor::BottomLeft, {6, 25});

    // rightSideBG3
    Build<CCScale9Sprite>::create("square02_small.png").scale(0.85f).opacity(50).contentSize(144,55).anchorPoint(1,0.5).with([this, objectData](auto node) {
        Build<CCLabelBMFont>::create("Rate It!", "bigFont.fnt").scale(0.35f).parentAtPos(node, Anchor::Top, {0, -10});
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
        node->addChildAtPosition(starsNode, Anchor::Top, {0, -32});

        std::ostringstream oss;
        oss << std::fixed << std::setprecision(2) << objectData.rating;
        Build<CCLabelBMFont>::create(fmt::format("{} ({})", oss.str(), objectData.ratingCount).c_str(), "bigFont.fnt").with([objectData](auto ratingLbl) {
            for (int i = 0; i < 4; i++) {
                // why doesnt CCFontSprite EXIST!?
                auto fontSpr = static_cast<CCSprite*>(ratingLbl->getChildren()->objectAtIndex(i));
                fontSpr->setColor(ObjectItem::starColor(objectData.rating));
            }
        }).scale(0.3f).parentAtPos(node, Anchor::Bottom, {3, 9});

        Build<CCSprite>::createSpriteName("edit_vLine_001.png").rotation(90).parentAtPos(node, Anchor::Top, {0, -20});
    }).parentAtPos(m_mainLayer, Anchor::Right, {-7, -70});

    // bottomRightBG
    Build<CCScale9Sprite>::create("square02_small.png").scale(0.85f).opacity(50).contentSize(144,39).anchorPoint(1,0.5).with([this, objectData](auto node) {
        Build<CCLabelBMFont>::create(std::to_string(objectData.downloads).c_str(), "bigFont.fnt").anchorPoint(0.5, 0).limitLabelWidth(50.0f, 0.32f, 0.1f).parentAtPos(node, Anchor::Bottom, {-25, 1}).store(m_downloadsLabel);
        Build<CCLabelBMFont>::create(std::to_string(objectData.favorites).c_str(), "bigFont.fnt").anchorPoint(0.5, 0).limitLabelWidth(50.0f, 0.32f, 0.1f).parentAtPos(node, Anchor::Bottom, {25, 1}).store(m_favoritesLabel);
    }).parentAtPos(m_mainLayer, Anchor::BottomRight, {-7, 25});

    Build<CCSprite>::createSpriteName("GJ_downloadBtn_001.png").scale(0.55f).intoMenuItem(
        this, menu_selector(ObjectPopup::onDownloadBtn)
    ).parentAtPos(m_buttonMenu, Anchor::BottomRight, {-90, 29});
    Build<CCMenuItemToggler>::createToggle(
        Build<CCSprite>::createSpriteName("gj_heartOff_001.png").scale(0.75f),
        Build<CCSprite>::createSpriteName("gj_heartOn_001.png").scale(0.75f),
        [this](auto node) {
            ObjectPopup::onFavBtn(node);
        }
    ).parentAtPos(m_buttonMenu, Anchor::BottomRight, {-48, 29}).toggle(Utils::arrayIncludes(m_workshop->m_user.favorites, objectData.id));
    Build<CCSprite>::createSpriteName("GJ_chatBtn_001.png").scale(0.65f).intoMenuItem([this]() {
        CommentsPopup::create(m_object, m_user)->show();
    }).parentAtPos(m_buttonMenu, Anchor::BottomRight, {-3, 3});

    this->setID("ObjectPopup"_spr);

    if (m_user.role < 2) return true;
    // Staff Bar
    CCSize contentSizeLeftBar = {70, this->m_bgSprite->getContentHeight()};
    Build<NineSlice>::create("GJ_square01.png").contentSize(contentSizeLeftBar).child(Build<CCMenu>::create().anchorPoint(0,0).pos(0,-5).contentSize(contentSizeLeftBar).with([this, objectData](auto menu) {
        Build<CCLabelBMFont>::create("Actions", "goldFont.fnt").scale(0.55f).parentAtPos(menu, Anchor::Top, {0, -15});
        if (objectData.status == 0) {
            Build<CCSprite>::createSpriteName("GJ_likeBtn_001.png").scale(0.55f).intoMenuItem([this]{
                /*auto popup = VotePopup::create("Review", [this](bool review) {
                    sendRequest(fmt::format("{}/objects/{}/{}", HOST_URL, m_object.id, (review) ? "accept" : "reject"));
                });
                popup->setWarning(, "Are you sure you want to <cy>reject this object</c>?");
                popup->show();*/
                geode::createQuickPopup("Warning", "Are you sure you want to <cy>accept this object</c>?", "No", "Yes", [this](auto, bool btn2) {
                    if (btn2) {
                        sendRequest(fmt::format("{}/objects/{}/{}", HOST_URL, m_object.id, "accept"));
                    }
                });
            }).parent(menu);
            Build<CCSprite>::createSpriteName("GJ_dislikeBtn_001.png").scale(0.55f).intoMenuItem([this]{ 
                ReportPopup::create(m_object, ReportActionType::Review, [this]() {
                    if (m_workshop != nullptr) {
                        this->onClose(nullptr);
                        m_workshop->RegenCategory();
                    }
                })->show();
            }).parent(menu);
        }
        Build<CCSprite>::createSpriteName((m_user.role < 3) ? "GJ_starBtn2_001.png" : "GJ_starBtnMod_001.png").scale(0.55f).intoMenuItem([this]{ 
            if ((m_user.role < 3)) return;
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
                    sendRequest(fmt::format("{}/objects/{}/feature", HOST_URL, m_object.id));
                }
            });
        }).enabled((m_user.role >= 3)).parent(menu);
        // add a disable for this
        Build<CCSprite>::createSpriteName("accountBtn_blocked_001.png").scale(0.55f).intoMenuItem([this]{ 
            ReportPopup::create(m_object, ReportActionType::ReviewAndCase, [this]() {
                if (m_workshop != nullptr) {
                    this->onClose(nullptr);
                    m_workshop->RegenCategory();
                }
            })->show();
        }).parent(menu);
        menu->setLayout(ColumnLayout::create()->setAxisAlignment(AxisAlignment::End)
            ->setCrossAxisAlignment(AxisAlignment::Center)
            ->setAutoScale(false)
            ->setCrossAxisOverflow(false)
            ->setGap(5)
            ->setGrowCrossAxis(false)
            ->setAxisReverse(true)
        );
        menu->updateLayout();
    })).parentAtPos(m_mainLayer, Anchor::Left, {-65, 0});
    generateContext();
    return true;
}

void ObjectPopup::onSliderZoom(CCObject*) {
    float value = m_slider->getValue();
    if (m_previewBG != nullptr) m_previewBG->setZoom(value * (float)MAX_ZOOM);
}

void ObjectPopup::generateContext() {
    m_listener.cancel();
    CCSize contentSizeRightBar = {115, this->m_bgSprite->getContentHeight()};
    Build<NineSlice>::create("GJ_square01.png").contentSize(contentSizeRightBar).child(Build<CCMenu>::create().anchorPoint(0,0).pos(0,10).contentSize({115,260}).with([this](auto menu) {
        Build<CCLabelBMFont>::create("Context", "goldFont.fnt").scale(0.8f).parentAtPos(menu, Anchor::Top, {0, -15});
        Build<CCLabelBMFont>::create("User", "bigFont.fnt").scale(0.6f).parentAtPos(menu, Anchor::Top, {0, -35});
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
        auto textAreaCase = MDTextArea::create("User has received no new cases.", {100, 85});
        auto textAreaHistory = MDTextArea::create("**Loading...**", {100, 85});
        m_listener.spawn(
            req.post(fmt::format("{}/objects/{}/context", HOST_URL, m_object.id)),
            [this, textAreaCase, textAreaHistory](web::WebResponse value) {
                auto jsonRes = value.json().unwrapOrDefault();
                if (Utils::notifError(jsonRes)) return;
                if (jsonRes.contains("user")) {
                    textAreaCase->setString(jsonRes.get("user").unwrap().asString().unwrapOrDefault().c_str());
                }
                if (jsonRes.contains("history")) {
                    textAreaHistory->setString(jsonRes.get("history").unwrap().asString().unwrapOrDefault().c_str());
                }
            }
        );
        menu->addChildAtPosition(textAreaCase, Anchor::Center, {0, 30});
        // a quick summary of the user, including stuff like if they have been warned
        Build<CCLabelBMFont>::create("History", "bigFont.fnt").scale(0.6f).parentAtPos(menu, Anchor::Center, {0, -15});
        menu->addChildAtPosition(textAreaHistory, Anchor::Center, {0, -75});
        menu->setLayout(ColumnLayout::create()->setAxisAlignment(AxisAlignment::Between)
            ->setCrossAxisAlignment(AxisAlignment::Center)
            ->setAutoScale(false)
            ->setCrossAxisOverflow(false)
            ->setGap(5)
            ->setGrowCrossAxis(false)
            ->setAxisReverse(true)
        );
        menu->updateLayout();
        // info about the object history, like if it was featured, or if this is an appeal from rejection
    })).parentAtPos(m_mainLayer, Anchor::Right, {65, 0});
}

void ObjectPopup::onRateBtn(CCObject* sender) {
    if (!m_user.authenticated) return FLAlertLayer::create("Error", "You cannot rate objects as you are <cy>not authenticated!</c>", "OK")->show();
    m_listener.cancel();
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
            if (numFromString<int>(item->getID()).unwrapOr(0) <= numFromString<int>(menuItem->getID()).unwrapOr(0)) {
                starFull->setVisible(true);
            } else {
                starEmpty->setVisible(true);
            }
        }
    }
    web::WebRequest req = web::WebRequest();
    req.userAgent(USER_AGENT);
    auto certValid = Mod::get()->getSettingValue<bool>("cert-valid");
    if (!certValid) {
        req.certVerification(certValid);
    }
    auto myjson = matjson::Value();
    myjson.set("token", m_token);
    myjson.set("stars", numFromString<int>(menuItem->getID()).unwrapOr(0));
    req.header("Content-Type", "application/json");
    req.bodyJSON(myjson);
    m_listener.spawn(
        req.post(fmt::format("{}/objects/{}/rate", HOST_URL, m_object.id)),
        [this](web::WebResponse value) {
            auto jsonRes = value.json().unwrapOrDefault();
            if (Utils::notifError(jsonRes)) return;
            Notification::create("Rated!", NotificationIcon::Success)->show();
        }
    );
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
        m_listener.cancel();
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
        m_listener.spawn(
            req.post(fmt::format("{}/objects/{}/download", HOST_URL, m_object.id)),
            [this](web::WebResponse value) {
                if (value.json().isErr()) return log::error("Response is not JSON.");
                auto jsonRes = value.json().unwrapOrDefault();
                if (Utils::notifError(jsonRes)) return;
                if (m_downloadsLabel != nullptr) m_downloadsLabel->setString(std::to_string(m_object.downloads).c_str());
            }
        );
    }
}

void ObjectPopup::onFavBtn(CCObject*) {
    if (!m_user.authenticated) return FLAlertLayer::create("Error", "You cannot favorite levels as you are <cy>not authenticated!</c>", "OK")->show();
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
        req.post(fmt::format("{}/objects/{}/favorite", HOST_URL, m_object.id)),
        [this](web::WebResponse value) {
            auto jsonRes = value.json().unwrapOrDefault();
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
            if (m_favoritesLabel != nullptr) m_favoritesLabel->setString(std::to_string(m_object.favorites).c_str());
        }
    );
}

void ObjectPopup::sendRequest(std::string url, bool exit) {
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
        req.post(url),
        [this, exit](web::WebResponse value) {
            auto jsonRes = value.json().unwrapOrDefault();
            if (Utils::notifError(jsonRes)) return;
            auto message = jsonRes.get("message");
            if (message.isOk()) {
                Notification::create(message.unwrap().asString().unwrapOrDefault(), NotificationIcon::Success)->show();
            } else {
                log::error("Unknown response, expected message. {}", message.err());
                Notification::create("Got an unknown response, check logs for details.", NotificationIcon::Warning)->show();
            }
            if (exit) {
                if (m_workshop != nullptr) {
                    this->onClose(nullptr);
                    m_workshop->RegenCategory();
                }
            }
        }
    );
}
