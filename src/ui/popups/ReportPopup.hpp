#pragma once

#include <Geode/ui/Popup.hpp>
#include "../../nodes/ObjectItem.hpp"
#include <Geode/utils/web.hpp>
using namespace geode::prelude;

class ReportPopup : public geode::Popup<ObjectData> {
protected:
    EventListener<web::WebTask> m_listener;
    ObjectData m_object;
    bool setup(ObjectData obj) override;
    TextInput* m_reportInput;
    void onReportBtn(CCObject*);
public:
    static ReportPopup* create(ObjectData obj) {
        auto ret = new ReportPopup();
        if (ret->initAnchored(350.f, 100.f, obj)) {
            ret->autorelease();
            return ret;
        }

        delete ret;
        return nullptr;
    }
};
