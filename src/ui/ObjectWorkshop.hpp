#pragma once
#include <Geode/utils/web.hpp>
#include <Geode/Geode.hpp>
#include "../nodes/ObjectItem.hpp"

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
    void setupCustomMenu(EditButtonBar* bar, bool hideItems);
    void onBackLbl(CCObject*);
    void onWorkshop(CCObject*);
    //void onCreateButton(CCObject*);
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
};

// do i really need a bunch of event listeners
class ObjectWorkshop : public geode::Popup<bool> {
protected:
    //CCScale9Sprite* leftBar;
    std::unordered_set<std::string> m_availableTags;
    EventListener<web::WebTask> m_listener2;
    EventListener<web::WebTask> m_listener1;
    EventListener<web::WebTask> m_listener0;
    EventListener<web::WebTask> m_tagsListener;
    bool m_authenticated = false;
    int m_amountItems = 0;
    std::string m_token;
    UserData m_user;
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

    LoadingCircle* loadingCircle;
    TextInput* m_searchInput;
    TextInput* m_pageInput;
    bool isSearching = false;

    bool setup(bool authenticated) override;
    void onInfoBtn(CCObject*);
    void onSearchBtn(CCObject*);
    void onFilterBtn(CCObject*);
    void onReloadBtn(CCObject*);
    void onPendingBtn(CCObject*);
    void createCategoryBtn(const char* string, int menuIndex);
    void onSideButton(CCObject*);
    void showProfile(int userID, bool self);
    void RegenCategory();
    void load();

    void onUploadBtn(CCObject*);
    void onRetryBtn(CCObject*);
    void onClickObject(CCObject*);
    void onBackBtn(CCObject*);

    CCScale9Sprite* rightBg;
    CCScrollLayerExt* m_scrollLayer;
    CCMenu* m_content;
    
    ObjectData m_currentObject;
    CCNode* objectInfoNode;
    CCMenuItemSpriteExtra* obj_backBtn;
    EventListener<web::WebTask> m_rateListener;
    EventListener<web::WebTask> m_favoriteListener;
    EventListener<web::WebTask> m_downloadListener;
    EventListener<web::WebTask> m_editListener;
    EventListener<web::WebTask> m_deleteListener;
    EventListener<web::WebTask> m_reviewListener;
    
    CCLabelBMFont* downloadsLabel;
    CCLabelBMFont* favoritesLabel;

    void onRateBtn(CCObject*);
    void actuallyDownload();
    void onDownloadBtn(CCObject*);
    void onFavBtn(CCObject*);
    void onDescBtn(CCObject*);
    void onTrashBtn(CCObject*);
    void onEditBtn(CCObject*);
    void onVerifyBtn(CCObject*);
    void onRejectBtn(CCObject*);
    void onReportBtn(CCObject*);

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
