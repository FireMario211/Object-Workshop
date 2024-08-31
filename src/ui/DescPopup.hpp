#pragma once

#include <Geode/ui/Popup.hpp>
#include "../nodes/ObjectItem.hpp"
using namespace geode::prelude;

class DescPopup : public geode::Popup<ObjectData> {
protected:
    bool setup(ObjectData obj) override;
public:
    static DescPopup* create(ObjectData obj) {
        auto ret = new DescPopup();
        if (ret->initAnchored(350.f, 200.f, obj)) {
            ret->autorelease();
            return ret;
        }

        delete ret;
        return nullptr;
    }
};
