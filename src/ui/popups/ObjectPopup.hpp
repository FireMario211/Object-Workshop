#pragma once

#include <Geode/ui/Popup.hpp>
using namespace geode::prelude;

#include "../../nodes/ObjectItem.hpp"
#include "../ObjectWorkshop.hpp"

class ObjectPopup : public geode::Popup {
protected:
    std::string m_token;
    async::TaskHolder<geode::utils::web::WebResponse> m_listener;
    ObjectWorkshop* m_workshop;

    CCLabelBMFont* downloadsLabel;
    CCLabelBMFont* favoritesLabel;
    ObjectData m_object;
    UserData m_user;

    ExtPreviewBG* m_previewBG;
    Slider* m_slider;

    float m_oldSliderValue;
    void onSliderZoom(CCObject*);
    void onZoomIn(CCObject*);
    void onZoomOut(CCObject*);
    void onResetZoom(CCObject*);

    bool init(ObjectData, UserData);

    void onAuthorBtn(CCObject*);
    void onRateBtn(CCObject*);
    void actuallyDownload();
    void onDownloadBtn(CCObject*);
    void onFavBtn(CCObject*);
    void onCommentsBtn(CCObject*);

    void onInfoBtn(CCObject*);
    void onTrashBtn(CCObject*);
    void onEditBtn(CCObject*);
    void onReviewBtn(CCObject*);
    void onFeatureBtn(CCObject*);
    void onReportBtn(CCObject*);
    void sendRequest(std::string url, bool exit = true);
public:
    virtual void onClose(CCObject* sender) override;
    ObjectWorkshop* getWorkshop() {
        return m_workshop;
    }
    ObjectData getObject() {
        return m_object;
    }
    static ObjectPopup* create(ObjectData object, UserData user) {
        auto ret = new ObjectPopup();
        if (ret->init(object, user)) {
            ret->autorelease();
            return ret;
        }

        delete ret;
        return nullptr;
    }
};
