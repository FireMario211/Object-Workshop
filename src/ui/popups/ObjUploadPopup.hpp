#pragma once
#include <Geode/ui/Popup.hpp>
using namespace geode::prelude;
#include "../ObjectWorkshop.hpp"
class ObjUploadPopup : public geode::Popup {
protected:
    std::string m_token;
    async::TaskHolder<geode::utils::web::WebResponse> m_listener;

    int m_selectedIndex = UINT16_MAX;
    ScrollLayerExt* m_scrollLayer;
    CCMenu* m_content;
    CCScale9Sprite* m_bg;

    ExtPreviewBG* m_previewBG;
    CCScale9Sprite* m_sliderBg;
    Slider* m_slider;

    float m_oldSliderValue;
    void onSliderZoom(CCObject*) {
        float value = m_slider->getValue();
        if (m_previewBG != nullptr) m_previewBG->setZoom(value * (float)MAX_ZOOM);
    }

    TextInput* m_objName;
    TextInput* m_objDesc;
    CCMenuItemSpriteExtra* m_filterBtn;
    CCMenuItemSpriteExtra* m_uploadBtn;
    std::unordered_set<std::string> m_filterTags;

    CCMenuItemSpriteExtra* m_prevBtn;
    CCMenuItemSpriteExtra* m_nextBtn;
    bool init(UserData);
    void showUpload();
    void onSelectObject(CCObject*);
    void onSelectPickPage(CCObject*);
    void onObjectDetailsPage(CCObject*);

public:
    static ObjUploadPopup* create(UserData user) {
        auto ret = new ObjUploadPopup();
        if (ret->init(user)) {
            ret->autorelease();
            return ret;
        }

        delete ret;
        return nullptr;
    }
};
