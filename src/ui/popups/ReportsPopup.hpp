#pragma once

#include "ObjectPopup.hpp"
#include <Geode/ui/Popup.hpp>
using namespace geode::prelude;

#include "../../nodes/ObjectItem.hpp"

class ReportsPopup : public geode::Popup<std::vector<ReportData>, UserData> {
protected:
    EventListener<web::WebTask> m_listener;

    bool setup(std::vector<ReportData>, UserData) override;

public:
    //virtual void onClose(CCObject* sender) override;
    static ReportsPopup* create(std::vector<ReportData> data, UserData user) {
        auto ret = new ReportsPopup();
        if (ret->initAnchored(250.f, 200.f, data, user)) {
            ret->autorelease();
            return ret;
        }

        delete ret;
        return nullptr;
    }
};
