#pragma once

#include <Geode/ui/Popup.hpp>
using namespace geode::prelude;

// definitely not copied from geode
class FiltersPopup : public geode::Popup<std::unordered_set<std::string>, std::unordered_set<std::string>, bool, utils::MiniFunction<void(std::unordered_set<std::string>)>> {
protected:
    CCMenu* m_tagsMenu;
    std::unordered_set<std::string> m_selectedTags;
    utils::MiniFunction<void(std::unordered_set<std::string>)> m_callback;

    bool setup(std::unordered_set<std::string> tags, std::unordered_set<std::string> selectedTags, bool uploading, utils::MiniFunction<void(std::unordered_set<std::string>)> callback) override;
    void onResetBtn(CCObject*);
    void onSelectTag(CCObject*);
    void updateTags();
    virtual void onClose(CCObject* sender) override;
public:
    static CCNode* createTags(std::unordered_set<std::string> tags);
    static FiltersPopup* create(std::unordered_set<std::string> tags, std::unordered_set<std::string> selectedTags, bool uploading, utils::MiniFunction<void(std::unordered_set<std::string>)> callback) {
        auto ret = new FiltersPopup();
        if (ret->initAnchored(350.f, 170.f, tags, selectedTags, uploading, callback)) {
            ret->autorelease();
            return ret;
        }

        delete ret;
        return nullptr;
    }
};
