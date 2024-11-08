#pragma once

#include <Geode/ui/Popup.hpp>
using namespace geode::prelude;

#include "../../nodes/ObjectItem.hpp"
#include "../ObjectWorkshop.hpp"

class ObjectPopup : public geode::Popup<ObjectData, UserData> {
protected:
    std::string m_token;
    EventListener<web::WebTask> m_listener;
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

    bool setup(ObjectData, UserData) override;

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
        if (ret->initAnchored(300.f, 275.f, object, user)) {
            ret->autorelease();
            return ret;
        }

        delete ret;
        return nullptr;
    }
};
