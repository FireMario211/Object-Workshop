#pragma once

#include <Geode/ui/Popup.hpp>
#include "../../nodes/ObjectItem.hpp"
#include "../ObjectWorkshop.hpp"
#include "../../nodes/TextInputNode.hpp"
#include <Geode/utils/web.hpp>
using namespace geode::prelude;

class EditPopup : public geode::Popup {
protected:
    async::TaskHolder<geode::utils::web::WebResponse> m_listener;
    ObjectData m_object;
    UserData m_user;
    std::unordered_set<std::string> m_availableTags;
    bool init(ObjectData obj, std::unordered_set<std::string> availableTags, UserData user);
    TextInput* m_objName;
    //TextInput* m_objDesc;
    TextInputNode* m_objDesc;
    CCScale9Sprite* m_previewBG;
    MDTextArea* m_overwriteInfo;
    void onUpdateBtn(CCObject*);
    void onOverwriteBtn(CCObject*);
    void updateDescObj(std::string);
    std::string m_oldObjectString;
    virtual void onClose(CCObject* sender) override {
        m_objDesc->cancel = true;
        Popup::onClose(sender);
    }
public:
    static EditPopup* create(ObjectData obj, std::unordered_set<std::string> availableTags, UserData user) {
        auto ret = new EditPopup();
        if (ret->init(obj, availableTags, user)) {
            ret->autorelease();
            return ret;
        }

        delete ret;
        return nullptr;
    }
};
