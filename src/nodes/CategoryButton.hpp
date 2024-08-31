#pragma once
#include <Geode/Geode.hpp>

using namespace geode::prelude;

class CategoryButton : public CCScale9Sprite {
    protected:
        virtual bool init(const char* string);
        CCScale9Sprite* bgSpr;
        CCScale9Sprite* bgSprBehind;
        CCScale9Sprite* activeSideIndicator;
        CCScale9Sprite* inactiveSideIndicator;
    public:
        void setIndicatorState(bool enabled);
        static CategoryButton* create(const char* string);
};
