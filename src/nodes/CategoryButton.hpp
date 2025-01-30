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
        void setDisabled(bool disabled) {
            ccColor3B bgSprCol = {86, 48, 29};
            ccColor3B bgSprBehindCol = {62, 35, 20};
            if (disabled) {
                bgSprCol.r -= 20;
                bgSprCol.g -= 20;
                bgSprCol.b -= 20;
                bgSprBehindCol.r -= 20;
                bgSprBehindCol.g -= 20;
                bgSprBehindCol.b -= 20;
            }
            bgSpr->setColor(bgSprCol);
            bgSprBehind->setColor(bgSprBehindCol);
        }
        void setIndicatorState(bool enabled);
        static CategoryButton* create(const char* string);
};
