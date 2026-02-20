#pragma once

#include <Geode/ui/Popup.hpp>
#include "../ObjectWorkshop.hpp"
#include "../../config.hpp"

using namespace geode::prelude;

class NewCasePopup : public geode::Popup {
protected:
    async::TaskHolder<geode::utils::web::WebResponse> m_listener;
    std::vector<std::string> caseTypes = {"Warning", "Upload Ban (T)", "Comment Ban (T)", "Account Ban (T)", "Upload Ban (P)", "Comment Ban (P)", "Account Ban (P)"};
    int caseCurrentIndex = 0;
    
    void updateCaseIndex(bool next) {
        if (!next) {
            caseCurrentIndex--;
            if (caseCurrentIndex <= 0) caseCurrentIndex = caseTypes.size() - 1;
        } else {
            caseCurrentIndex++;
            if (caseCurrentIndex >= (caseTypes.size())) caseCurrentIndex = 0;
        }
    }

    bool init(UserData, std::function<void()>);

    virtual void onClose(CCObject* sender) override;
public:
    static NewCasePopup* create(UserData user, std::function<void()> callback) {
        auto ret = new NewCasePopup();
        if (ret->init(user, callback)) {
            ret->autorelease();
            return ret;
        }
        delete ret;
        return nullptr;
    }
};
