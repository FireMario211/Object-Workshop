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

    CCLabelBMFont* m_downloadsLabel;
    CCLabelBMFont* m_favoritesLabel;
    ObjectData m_object;
    UserData m_user;

    ExtPreviewBG* m_previewBG;
    Slider* m_slider;

    float m_oldSliderValue;
    void onSliderZoom(CCObject*);
    void onZoomIn(CCObject*);
    void onZoomOut(CCObject*);

    bool init(ObjectData, UserData);

    void onRateBtn(CCObject*);
    void actuallyDownload();
    void onDownloadBtn(CCObject*);
    void onFavBtn(CCObject*);

    void sendRequest(std::string url, bool exit = true);
    void generateContext();
public:
    virtual void onClose(CCObject* sender) override {
        Popup::onClose(sender);
    }
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
