#pragma once

#include <Geode/ui/Popup.hpp>
using namespace geode::prelude;

// definitely not copied from geode
class FiltersPopup : public geode::Popup {
protected:
    CCMenuItemToggler* m_pendingBtn;
    CCMenuItemToggler* m_reportsBtn;
    CCMenu* m_tagsMenu;
    std::unordered_set<std::string> m_selectedTags;
    std::function<void(std::unordered_set<std::string>, bool, bool)> m_callback;

    bool init(std::unordered_set<std::string> tags, std::unordered_set<std::string> selectedTags, int role, bool uploading, std::function<void(std::unordered_set<std::string>, bool, bool)> callback);
    void onResetBtn(CCObject*);
    void onSelectTag(CCObject*);
    void updateTags();
    virtual void onClose(CCObject* sender) override;
public:
    static CCNode* createTags(std::unordered_set<std::string> tags, CCSize contentSize = {105, 27}, CCPoint anchorPoint = {0, 0.5}, AxisAlignment alignment = AxisAlignment::Start);
    static FiltersPopup* create(std::unordered_set<std::string> tags, std::unordered_set<std::string> selectedTags, int role, bool uploading, std::function<void(std::unordered_set<std::string>, bool, bool)> callback) {
        auto ret = new FiltersPopup();
        if (ret->init(tags, selectedTags, role, uploading, callback)) {
            ret->autorelease();
            return ret;
        }

        delete ret;
        return nullptr;
    }
};
