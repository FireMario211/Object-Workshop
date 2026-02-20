#pragma once
#include "ObjectPopup.hpp"
#include <Geode/ui/Popup.hpp>
#include "Geode/utils/web.hpp"
using namespace geode::prelude;
#include "../../nodes/ObjectItem.hpp"
class ReportsPopup : public geode::Popup {
protected:
    async::TaskHolder<geode::utils::web::WebResponse> m_listener;
    bool init(std::vector<ReportData>, UserData);
public:
    //virtual void onClose(CCObject* sender) override;
    static ReportsPopup* create(std::vector<ReportData> data, UserData user) {
        auto ret = new ReportsPopup();
        if (ret->init(data, user)) {
            ret->autorelease();
            return ret;
        }
        delete ret;
        return nullptr;
    }
};
