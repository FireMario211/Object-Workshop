#pragma once
#include <Geode/ui/Popup.hpp>
#include "../../nodes/ObjectItem.hpp"
#include <Geode/utils/web.hpp>
#include "../../nodes/ComboComponent.hpp"
using namespace geode::prelude;

enum ReportActionType {
    Report,
    Review,
    ReviewAndCase,
    Appeal
};

class ReportPopup : public geode::Popup {
protected:
    async::TaskHolder<geode::utils::web::WebResponse> m_listener;
    ObjectData m_object;
    bool init(ObjectData obj, ReportActionType type);
    TextInput* m_reportInput;
    ComboComponentNode* m_combo;
    CCMenuItemToggler* m_forceReject;
    void onReportBtn(CCObject*);
    size_t m_selectedIndex;
    ReportActionType m_type;
    geode::Function<void()> m_callback;
public:
    static ReportPopup* create(ObjectData obj, ReportActionType type) {
        auto ret = new ReportPopup();
        if (ret->init(obj, type)) {
            ret->autorelease();
            return ret;
        }

        delete ret;
        return nullptr;
    }
    static ReportPopup* create(ObjectData obj, ReportActionType type, geode::Function<void()> callback) {
        auto ret = new ReportPopup();
        ret->m_callback = std::move(callback);
        if (ret->init(obj, type)) {
            ret->autorelease();
            return ret;
        }
        delete ret;
        return nullptr;
    }
};
