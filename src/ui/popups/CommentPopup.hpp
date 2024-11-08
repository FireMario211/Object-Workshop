#pragma once

#include <Geode/ui/Popup.hpp>
#include "../../nodes/ObjectItem.hpp"
#include "../../nodes/TextInputNode.hpp"
#include <Geode/utils/web.hpp>
using namespace geode::prelude;

class CommentPopup : public geode::Popup<ObjectData, std::function<void()>> {
protected:
    EventListener<web::WebTask> m_listener;
    ObjectData m_object;
    CCLabelBMFont* m_charCountLabel;
    std::string m_descText;
    TextInputNode* m_inputNode;

    bool m_closed = false; // prevent unnecessary crash because for some reason this can happen

    bool setup(ObjectData obj, std::function<void()>) override;
    void onSubmit(CCObject*);
    void updateCharCountLabel();
    void updateDescText(std::string string);
    std::function<void()> m_submitCallback;
    
    virtual void onClose(CCObject* sender) override;
public:
    static CommentPopup* create(ObjectData obj, std::function<void()> callback) {
        auto ret = new CommentPopup();
        if (ret->initAnchored(400.f, 135.f, obj, callback)) {
            ret->autorelease();
            return ret;
        }
        delete ret;
        return nullptr;
    }
};
