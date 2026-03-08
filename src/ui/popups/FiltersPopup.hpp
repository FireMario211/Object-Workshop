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
    bool m_selectedFeatured;
    std::function<void(std::unordered_set<std::string>, bool, bool, bool)> m_callback;

    bool init(std::unordered_set<std::string> tags, std::unordered_set<std::string> selectedTags, bool selectedFeatured, int role, bool uploading, std::function<void(std::unordered_set<std::string>, bool, bool, bool)> callback);
    void onSelectTag(CCObject*);
    void updateTags();
    virtual void onClose(CCObject* sender) override;
public:
    static CCNode* createTags(std::unordered_set<std::string> tags, CCSize contentSize = {105, 27}, CCPoint anchorPoint = {0, 0.5}, AxisAlignment alignment = AxisAlignment::Start);
    static FiltersPopup* create(std::unordered_set<std::string> tags, std::unordered_set<std::string> selectedTags, bool selectedFeatured, int role, bool uploading, std::function<void(std::unordered_set<std::string>, bool, bool, bool)> callback) {
        auto ret = new FiltersPopup();
        if (ret->init(tags, selectedTags, selectedFeatured, role, uploading, callback)) {
            ret->autorelease();
            return ret;
        }

        delete ret;
        return nullptr;
    }
};
