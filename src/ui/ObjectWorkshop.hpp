#pragma once
#include <Geode/utils/web.hpp>
#include <Geode/Geode.hpp>
#include <Geode/ui/Notification.hpp>
#include <Geode/ui/Popup.hpp>
#include <Geode/ui/TextInput.hpp>
#include "../nodes/ObjectItem.hpp"
#include "../nodes/ScrollLayerExt.hpp"
#include "../nodes/ExtPreviewBG.hpp"
#include "../nodes/ScrollLayerExt.hpp"

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
    void onLoadComments(CCObject*);
    void onLeftCommentPage(CCObject*);
    void onRightCommentPage(CCObject*);
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
    EventListener<web::WebTask> m_userListener;
    EventListener<web::WebTask> m_rateListener;
    EventListener<web::WebTask> m_favoriteListener;
    EventListener<web::WebTask> m_downloadListener;
    EventListener<web::WebTask> m_editListener;
    EventListener<web::WebTask> m_deleteListener;
    EventListener<web::WebTask> m_reviewListener;
    EventListener<web::WebTask> m_commentListener;
    EventListener<web::WebTask> m_topCommentsListener;
    
    CCLabelBMFont* downloadsLabel;
    CCLabelBMFont* favoritesLabel;
    CCLabelBMFont* commentsLabel;

    void onRateBtn(CCObject*);
    void actuallyDownload();
    void onDownloadBtn(CCObject*);
    void onFavBtn(CCObject*);
    void onCommentBtn(CCObject*);
    void onDescBtn(CCObject*);
    void onTrashBtn(CCObject*);
    void onEditBtn(CCObject*);
    void onVerifyBtn(CCObject*);
    void onRejectBtn(CCObject*);
    void onReportBtn(CCObject*);

    ExtPreviewBG* previewBG;
    void onZoomIn(CCObject*);
    void onZoomOut(CCObject*);
    void onResetZoom(CCObject*);

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
    EventListener<web::WebTask> m_uploadListener;

    virtual void onClose(CCObject*) override;

public:
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
};
