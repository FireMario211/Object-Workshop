#pragma once
#include <Geode/Geode.hpp>

using namespace geode::prelude;

class CategoryButton : public CCNode {
    protected:
        virtual bool init(const char* sprName, bool isSpriteFrame, bool bgVisible);
        CCSprite* m_bgSprBehind;
    public:
        void setIndicatorState(bool enabled);
        static CategoryButton* create(const char* sprName, bool isSpriteFrame, bool bgVisible);
        static CategoryButton* create(const char* sprName, bool isSpriteFrame) {
            return create(sprName, isSpriteFrame, true);
        }
};
