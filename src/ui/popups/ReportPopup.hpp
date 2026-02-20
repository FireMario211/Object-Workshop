#pragma once
#include <Geode/ui/Popup.hpp>
#include "../../nodes/ObjectItem.hpp"
#include <Geode/utils/web.hpp>
using namespace geode::prelude;
class ReportPopup : public geode::Popup {
protected:
    async::TaskHolder<geode::utils::web::WebResponse> m_listener;
    ObjectData m_object;
    bool init(ObjectData obj);
    TextInput* m_reportInput;
    void onReportBtn(CCObject*);
public:
    static ReportPopup* create(ObjectData obj) {
        auto ret = new ReportPopup();
        if (ret->init(obj)) {
            ret->autorelease();
            return ret;
        }

        delete ret;
        return nullptr;
    }
};
