#pragma once

#include <Geode/ui/Popup.hpp>
#include "../../nodes/ObjectItem.hpp"
#include "../ObjectWorkshop.hpp"
using namespace geode::prelude;

class CommentsPopup : public geode::Popup<ObjectData, UserData> {
protected:
    EventListener<web::WebTask> m_listener;
    ObjectData m_object;
    UserData m_user;
    CCMenu* filterMenu;
    int m_currentFilter = 1;
    CCLabelBMFont* pageLabel;
    CCScale9Sprite* m_commentsBG;

    CCMenuItemSpriteExtra* leftArrowBtn;
    CCMenuItemSpriteExtra* rightArrowBtn;

    bool setup(ObjectData obj, UserData user) override;
    void updateCategoryBG();
    
    void onLoadComments(CCObject*);

    void onCategoryButton(CCObject*);
public:
    virtual void onClose(CCObject* sender) override;
    static CommentsPopup* create(ObjectData obj, UserData user) {
        auto ret = new CommentsPopup();
        if (ret->initAnchored(300.f, 200.f, obj, user)) {
            ret->autorelease();
            return ret;
        }
        delete ret;
        return nullptr;
    }
};
