#pragma once

#include <Geode/ui/Popup.hpp>
#include "../../nodes/ObjectItem.hpp"
#include "../../nodes/TextInputNode.hpp"
#include <Geode/utils/web.hpp>
using namespace geode::prelude;

class CommentPopup : public geode::Popup, public EmotePickerDelegate {
protected:
    async::TaskHolder<geode::utils::web::WebResponse> m_listener;
    ObjectData m_object;
    CCLabelBMFont* m_charCountLabel;
    std::string m_descText;
    TextInputNode* m_inputNode;
    CCMenuItemSpriteExtra* m_emotesBtn;
    Ref<EmotePickerPopup> m_emotePicker;

    bool m_closed = false; // prevent unnecessary crash because for some reason this can happen

    bool init(ObjectData obj, std::function<void()>);
    void onSubmit(CCObject*);
    void onEmotes(CCObject*);
    void updateCharCountLabel();
    void updateDescText(std::string string);
    geode::Function<void()> m_submitCallback;
    
    virtual void emotePickerClosed(EmotePickerPopup* picker) override {
        if (picker == m_emotePicker) m_emotePicker = nullptr;
    }
    virtual void onClose(CCObject* sender) override;
public:
    static CommentPopup* create(ObjectData obj, std::function<void()> callback) {
        auto ret = new CommentPopup();
        if (ret->init(obj, callback)) {
            ret->autorelease();
            return ret;
        }
        delete ret;
        return nullptr;
    }
};
