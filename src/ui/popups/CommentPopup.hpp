#pragma once

#include <Geode/ui/Popup.hpp>
#include "../../nodes/ObjectItem.hpp"
#include <Geode/utils/web.hpp>
using namespace geode::prelude;

class CommentPopup : public geode::Popup<ObjectData, utils::MiniFunction<void()>>, TextInputDelegate {
protected:
    EventListener<web::WebTask> m_listener;
    ObjectData m_object;
    CCLabelBMFont* m_charCountLabel;
    std::string m_descText;
    bool setup(ObjectData obj, utils::MiniFunction<void()>) override;
    void onSubmit(CCObject*);
    void updateCharCountLabel();
    void updateDescText(std::string string);
    utils::MiniFunction<void()> m_submitCallback;
    
    void textInputOpened(CCTextInputNode*) override {}
    void textChanged(CCTextInputNode*) override;
    void textInputClosed(CCTextInputNode*) override;
public:
    static CommentPopup* create(ObjectData obj, utils::MiniFunction<void()> callback) {
        auto ret = new CommentPopup();
        if (ret->initAnchored(400.f, 135.f, obj, callback)) {
            ret->autorelease();
            return ret;
        }
        delete ret;
        return nullptr;
    }
};
