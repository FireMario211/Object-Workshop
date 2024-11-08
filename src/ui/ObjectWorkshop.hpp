#pragma once
#include <Geode/utils/web.hpp>
#include <Geode/Geode.hpp>
#include <Geode/ui/Notification.hpp>
#include <Geode/ui/Popup.hpp>
#include <Geode/ui/TextInput.hpp>
#include <cassert>
#include "../nodes/ObjectItem.hpp"
#include "../nodes/ScrollLayerExt.hpp"
#include "../nodes/ExtPreviewBG.hpp"
#include "../nodes/TextInputNode.hpp"
#include "../nodes/ScrollLayerExt.hpp"
#include "popups/WarningPopup.hpp"

using namespace geode::prelude;

#include <Geode/modify/EditorUI.hpp>

// youre probably thinking: wth are you doing!? treating it like a class you can define in a source file!? this is outrageous!
// anyways the definitions are in main.cpp
struct CustomObjects : Modify<CustomObjects, EditorUI> {
    struct Fields {
        CCMenuItemSpriteExtra* btn1;
        CCMenuItemSpriteExtra* btn2;
        CCMenuItemSpriteExtra* customObjsLabel;
        CCLabelBMFont* myObjsLabel;
        std::string currentObjString;
        EventListener<web::WebTask> m_listener;
        bool hasCheckedUploads;

        CCMenu* menu;
        bool m_hasMade = false;
    };
    bool init(LevelEditorLayer* editorLayer);
    void onWorkshop(CCObject*);
};

// since for whatever reason this isnt in headers...
class BreakLine : public CCNode {
protected:
    void draw() override {
        // some nodes sometimes set the blend func to
        // something else without resetting it back
        ccGLBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        ccDrawSolidRect({ 0, 0 }, this->getContentSize(), { 1.f, 1.f, 1.f, .2f });
        CCNode::draw();
    }

public:
    static BreakLine* create(float width) {
        auto ret = new BreakLine;
        if (ret->init()) {
            ret->autorelease();
            ret->setContentSize({ width, 1.f });
            return ret;
        }
        delete ret;
        return nullptr;
    }
};

struct UserData {
    int account_id;
    std::string name;
    matjson::Array downloaded;
    matjson::Array favorites;
    int uploads;
    int role;
    bool authenticated = false;
    matjson::Array icon;
    int featured;
};

// do i really need a bunch of event listeners
class ObjectWorkshop : public geode::Popup<bool>, public TextInputDelegate {
protected:
    std::unordered_set<std::string> m_availableTags;
    EventListener<web::WebTask> m_listener2;
    EventListener<web::WebTask> m_listener1;
    EventListener<web::WebTask> m_listener0;
    EventListener<web::WebTask> m_tagsListener;
    EventListener<web::WebTask> m_caseListener;
    bool m_authenticated = false;
    int m_amountItems = 0;
    std::string m_token;
    CCMenu* m_categoryButtons;

    CCNode* myUploadsBar;
    CCNode* categoryBar;
    CCMenu* myUploadsMenu;
    CCMenu* categoryItems;

    CCLabelBMFont* pageLabel;
    CCLabelBMFont* bottomPageLabel;
    int m_currentPage = 1;
    int m_maxPage = 1;
    void onLeftPage(CCObject*);
    void onRightPage(CCObject*);
    void textInputOpened(CCTextInputNode* input) override;
    void textInputClosed(CCTextInputNode* input) override;
    void keyDown(cocos2d::enumKeyCodes) override;
    virtual void keyBackClicked() override;

    LoadingCircle* loadingCircle;
    TextInput* m_searchInput;
    TextInput* m_pageInput;
    bool isSearching = false;

    bool setup(bool authenticated) override;
    void onProfileSelf(CCObject*);
    void onSearchBtn(CCObject*);
    void onFilterBtn(CCObject*);
    void onReloadBtn(CCObject*);
    void onPendingBtn(CCObject*);
    void createCategoryBtn(const char* string, int menuIndex);
    void onSideButton(CCObject*);
    void showProfile(int userID, bool self);
    void load();
    int m_currentMenu = 0; // 0 = objects, 1 = uploading, 2 = in object

    void onUploadBtn(CCObject*);
    void onRetryBtn(CCObject*);

    void onClickObject(CCObject*);
    int m_currentUserID;
    UserData m_currentUser;

    void onReviewerInfoBtn(CCObject*) {
        FLAlertLayer::create("About Reviewer", "This player is a <cg>Reviewer</c>!\n\nTheir job is to <cy>review</c> any <cl>pending objects</c> that other creators have uploaded to the workshop!", "OK")->show();
    }
    void onAdminInfoBtn(CCObject*) {
        FLAlertLayer::create("About Admin", "This player is an <cr>Administrator</c> or <cp>Developer</c>!", "OK")->show();
    }
    void onGoProfileBtn(CCObject*) {
        ProfilePage::create(m_currentUserID, false)->show();
    }
    void onAdminBtn(CCObject*);

    void onBackBtn(CCObject*);

    CCScale9Sprite* rightBg;
    ScrollLayerExt* m_scrollLayer;
    CCMenu* m_content;
    
    ObjectData m_currentObject;
    CCNode* objectInfoNode;
    CCMenuItemSpriteExtra* obj_backBtn;
    
    CCLabelBMFont* downloadsLabel;
    CCLabelBMFont* favoritesLabel;
    CCLabelBMFont* commentsLabel;

    void actuallyDownload();
    void onDownloadBtn(CCObject*);

    std::unordered_set<std::string> m_filterTags;
    void onUpload(CCObject*);
    void onUploadFilterBtn(CCObject*);
    void onRulesBtn(CCObject*) {
        FLAlertLayer::create(
            nullptr,
            "Rules",
            "1. Do not <cy>upload spam, duplicate, or useless objects.</c>\n2. Do not upload <cy>any stolen art</c>, objects that <cy>violate copyright</c>, or objects <cy>without the creators permission.</c>\n3. Do not upload objects that relate to anything that is <cy>inappropriate, explicit, sexual, or violent.</c>\n4. Use an <cy>appropriate name, and tags</c> when uploading.\n\nBreaking these rules will result in a <cr>temporary ban</c>, and possibly a <cr>permanent one</c> if <cy>too severe or repeated.</c>",
            "OK",
            nullptr,
            400.0F
        )->show();
    };
    TextInput* m_objName;
    TextInput* m_objDesc;
    //TextInputNode* m_objDesc;
    EventListener<web::WebTask> m_uploadListener;

    CCArray* m_oldCustomObjectButtonArray = nullptr;

    virtual void onClose(CCObject*) override;
    
    CaseData caseData;
    bool shownWarning = false;
    bool hasWarning = false;
    bool acknowledgedWarning = false;
    bool shownObjects = false;
public:
    

    bool m_inEditor;
    LevelEditorLayer* m_editorLayer;
    UserData m_user;
    void RegenCategory();
    void onClickUser(int accountID);
    std::unordered_set<std::string> getTags() { return m_availableTags; };
    static ObjectWorkshop* create(bool authenticated) {
        auto ret = new ObjectWorkshop();
        if (ret->initAnchored(425.f, 290.f, authenticated)) {
            ret->autorelease();
            return ret;
        }
        delete ret;
        return nullptr;
    }
    static ObjectWorkshop* createToUser(bool authenticated, int accountID) {
        auto ret = new ObjectWorkshop();
        if (ret->initAnchored(425.f, 290.f, authenticated)) {
            ret->autorelease();
            ret->onClickUser(accountID);
            return ret;
        }
        delete ret;
        return nullptr;
    }

    // reverse engineered funcs because ROBERT
    static CCSprite* spriteFromObjectString(gd::string objectString, bool center, bool filterObjects, int limit, CCArray* smartBlocks, CCArray* filter, GameObject* keyframeObject) {
        /*
        gd::string - object string, duh 
        bool - something with CCPoint
        bool - something with centering and deleting smart objects, 
        int - how many objects should render (0 = unlimited)
        cocos2d::CCArray* - something with smart blocks,
        cocos2d::CCArray* - i have no idea, maybe a filter of what objects to not show?
        GameObject* - something with keyframes
        */
        objectString = "1,3817,2,2925,3,285,108,3,21,9,24,7,128,2.08,129,2.08;1,211,2,2925,3,285,108,3,21,3;";
        auto editor = EditorUI::get();
        if (!editor) return nullptr;
        //return editor->spriteFromObjectString(objectString, center, filterObjects, limit, smartBlocks, filter, keyframeObject);
        auto sprite = CCSprite::create();
        CCArray* objectsArray = LevelEditorLayer::get()->createObjectsFromString(objectString, true, true);
        if (filter != nullptr && filterObjects) {
            for (int i = 0; (i < objectsArray->count()) && (i < filter->count()); i++) {
                auto object = as<GameObject*>(objectsArray->objectAtIndex(i));
                auto objectFiltered = as<GameObject*>(filter->objectAtIndex(i));
                int mainColorMode = object->getMainColorMode();
                if (editor->m_editorLayer->m_levelSettings->m_effectManager->colorExists(mainColorMode)) {
                    auto colorAction = editor->m_editorLayer->m_levelSettings->m_effectManager->getColorAction(mainColorMode);
                    object->updateMainColor(colorAction->m_fromColor);
                    object->m_baseColor->m_opacity = colorAction->m_fromOpacity;
                }
                if (objectFiltered->m_colorSprite != nullptr) {
                    auto colorAction = editor->m_editorLayer->m_levelSettings->m_effectManager->getColorAction(object->getSecondaryColorMode());
                    object->updateMainColor(colorAction->m_fromColor);
                    object->m_detailColor->m_opacity = colorAction->m_fromOpacity;
                }
                object->setOpacity(255);
                if (keyframeObject != nullptr) {
                    object->m_hasGroupParent = (objectFiltered == keyframeObject);
                }
            }
        }
        editor->repositionObjectsToCenter(objectsArray, {1000, 1000}, center);
        editor->deleteSmartBlocksFromObjects(objectsArray);
        editor->deleteTypeFromObjects(2065, objectsArray);
        if (limit > 0) {
            while (limit < objectsArray->count()) { //TODO: fix it to not be while loop because this seems dangerous!
                editor->m_editorLayer->removeObject(static_cast<GameObject*>(objectsArray->lastObject()), true);
                objectsArray->removeLastObject(true);
            }
        }
        // now the actual rendering!
        //
        float fVar16 = 0.0;
        float fVar15 = fVar16;
        float fVar18 = fVar16;
        float fVar17 = fVar16;
        for (int i = 0; (i < objectsArray->count()); i++) {
            auto object = as<GameObject*>(objectsArray->objectAtIndex(i));
            editor->m_editorLayer->removeObject(object, true);
            object->setPosition(object->getPosition()); // erm whats the point of this??
            sprite->addChild(object, (int)object->getObjectZLayer() * 10000 + object->getObjectZOrder());
            if (smartBlocks != nullptr) {
                smartBlocks->addObject(object);
            }
            object->updateBlendMode();
            if (!object->m_shouldBlendBase) {
                object->setBlendFunc({771, 1});
            } else {
                object->setBlendFunc({770, 1});
            }
            if (object->m_colorSprite != nullptr && !object->m_unk28c) {
                if (!object->m_shouldBlendDetail) {
                    object->setBlendFunc({771, 1});
                } else {
                    object->setBlendFunc({770, 1});
                }
                if (object->m_shouldBlendBase == object->m_shouldBlendDetail) {
                    object->addColorSpriteToSelf();
                } else {
                    ZLayer zLayer = object->getObjectZLayer();
                    if (object->m_shouldBlendBase && object->m_colorZLayerRelated) {
                        zLayer = (ZLayer)((int)zLayer + (int)ZLayer::B2);
                    }
                    int zOrder = object->getObjectZOrder();
                    object->m_colorSpriteLocked = false;
                    object->m_colorSprite->setScale(object->m_colorSprite->getScale());
                    object->m_colorSprite->setRotation(object->m_colorSprite->getRotation());
                    object->m_colorSprite->setPosition(object->getPosition());
                    object->m_colorSprite->removeFromParentAndCleanup(true);
                    sprite->addChild(object->m_colorSprite, (int)zLayer * 10000 + zOrder);
                }
            }
            object->getPosition();
            CCRect a;
            
            object->setCascadeColorEnabled(true);

            auto rect = object->getObjectRect();

            float minX = rect.getMinX();
            float maxX = rect.getMaxX();
            float minY = rect.getMinY();
            float maxY = rect.getMaxY();
            if ((fVar17 != 0.0) && (fVar17 <= minX)) {
              minX = fVar17;
            }
            fVar17 = minX;
            if ((fVar18 != 0.0) && (maxX <= fVar18)) {
              maxX = fVar18;
            }
            fVar18 = maxX;
            if ((fVar16 != 0.0) && (fVar16 <= minY)) {
              minY = fVar16;
            }
            fVar16 = minY;
            if ((fVar15 != 0.0) && (maxY <= fVar15)) {
              maxY = fVar15;
            }
            fVar15 = maxY;
        }
        // 120 36
        // 210 240
        CCSize content = {fVar18 - fVar17,fVar15 - fVar16};

        /*
        log::info("{} = {} | {} = {}", content.width, content.width == 210.F, content.height, content.height == 240.F);
        //log::info("step 6 {},{},{},{},{}", rect, rect.getMinX(), rect.getMinY(), rect.getMaxX(), rect.getMaxY());
        sprite->setContentSize(content);
        /]*sprite->setContentSize({210, 240});
        sprite->setPosition({120, 36});*]/
        for (int i = 0; i < objectsArray->count(); i++) {
            auto object = as<GameObject*>(objectsArray->objectAtIndex(i));
            sprite->convertToNodeSpace(object->getPosition());
            object->setPosition(content);
            object->setVisible(!object->m_isHide);
            object->setVisible(true); // @ CUSTOM ADDITION
        }*/
        return sprite;
    }
};
