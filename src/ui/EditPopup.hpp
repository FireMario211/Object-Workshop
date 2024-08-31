#pragma once

#include <Geode/ui/Popup.hpp>
#include "../nodes/ObjectItem.hpp"
#include <Geode/utils/web.hpp>
using namespace geode::prelude;

class EditPopup : public geode::Popup<ObjectData, std::unordered_set<std::string>> {
protected:
    EventListener<web::WebTask> m_listener;
    ObjectData m_object;
    std::unordered_set<std::string> m_availableTags;
    bool setup(ObjectData obj, std::unordered_set<std::string> availableTags) override;
    TextInput* m_objName;
    TextInput* m_objDesc;
    void onUpdateBtn(CCObject*);
    void onFilterBtn(CCObject*);
public:
    static EditPopup* create(ObjectData obj, std::unordered_set<std::string> availableTags) {
        auto ret = new EditPopup();
        if (ret->initAnchored(350.f, 160.f, obj, availableTags)) {
            ret->autorelease();
            return ret;
        }

        delete ret;
        return nullptr;
    }
};
