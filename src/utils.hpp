#pragma once

#include <Geode/Geode.hpp>

class Utils {
    public:
    // honestly matjson needs this tbh!
    template <typename T>
    static bool arrayIncludes(const matjson::Array& array, T value) {
        return std::find(array.begin(), array.end(), value) != array.end();
    }
    static cocos2d::ccColor3B generateColorFromString(const std::string& seed) {
        std::hash<std::string> hash_fn;
        size_t hash_value = hash_fn(seed);
        cocos2d::ccColor3B color;
        color.r = static_cast<GLubyte>((hash_value & 0xFF0000) >> 16);
        color.g = static_cast<GLubyte>((hash_value & 0x00FF00) >> 8);
        color.b = static_cast<GLubyte>(hash_value & 0x0000FF);
        return color;
    }
    /*template <typename T>
    static T convertValue(const matjson::Value& value) {
        return value.as_string();
        if (value.is_string()) {
            return value.as_string();
        } else if constexpr (std::is_same_v<T, double>) {
            result.insert(value.as_double());
        } else if (value.is_number()) {
            //return value.as_int();
        } else if (value.is_bool()) {
            //return value.as_bool();
        } else if (value.is_null()) {
            //return nullptr;
        } else {
            return value;
        }
    }*/
    static std::string convertValue(const matjson::Value& value) {
        return value.as_string();
    }
    template <typename T>
    static std::unordered_set<T> arrayToUnorderedSet(const matjson::Array& array) {
        std::unordered_set<T> result;
        for (const auto& value : array) {
            if (value.is_string()) {
                result.insert(value.as_string());
            }/* else if (value.is_number()) {
                result.insert(value.as_int());
            }*/
            //result.insert(convertValue<T>(value));
        }
        return result;
    };
    template <typename T>
    static matjson::Array unorderedSetToArray(std::unordered_set<T> set) {
        return matjson::Array(set.begin(), set.end());
    };
    static std::string url_encode(const std::string &value) {
        std::ostringstream escaped;
        escaped.fill('0');
        escaped << std::hex;

        for (char c : value) {
            if (isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~') {
                escaped << c;
            } else {
                escaped << '%' << std::setw(2) << int((unsigned char) c);
            }
        }

        return escaped.str();
    }
    static std::string menuIndexToString(int menuIndex) {
        switch (menuIndex) {
            case -1:
            default:
                return "Uploads";
            case 0:
                return "My Objects";
            case 1:
                return "Favorites";
            /*case 5:
                return "Trending";*/
            case 2:
                return "Top Downloads";
            case 3:
                return "Most Popular";
            case 4:
                return "Most Liked";
            case 5:
                //return "Trending";
                return "Featured";
            case 6:
                return "Most Recent";
            case 7:
                return "Pending";
            case 8:
                return "Reports";
        }
    }
    static int intToCategory(int menuIndex) {
        if (menuIndex != 5) {
            return menuIndex - 2;
        } else {
            return 5;
        }
    }
    static float calculateScale(const std::string& text, int minLength, int maxLength, float minScale, float maxScale) {
        int length = text.length();
        if (length <= minLength) {
            return minScale;
        } else if (length >= maxLength) {
            return maxScale;
        } else {
            float scale = minScale - ((length - minLength) * (minScale - maxScale) / (maxLength - minLength));
            return scale;
        }
    }
    static float calculateScale(int length, int minLength, int maxLength, float minScale, float maxScale) {
        if (length <= minLength) {
            return minScale;
        } else if (length >= maxLength) {
            return maxScale;
        } else {
            float scale = minScale - ((length - minLength) * (minScale - maxScale) / (maxLength - minLength));
            return scale;
        }
    }

    
    // ai generated because idk how to even think of this
    //
    /**
    -220 = (380, 325)
    -190 = (325, 290)
    -160 = (290, 245)

    increments of 30 for the contentYPos part, and increments of 35 for the ranges part
    **/
    static bool isInScrollSnapRange(int contentYPos, int x) {
        // Define the base values
        const int baseX = -220;
        const int baseY1 = 380;
        const int baseY2 = 325;
        const int xIncrement = 30;
        const int yIncrement = 35;

        // Calculate the number of steps from the base X
        int steps = std::round(static_cast<double>(x - baseX) / xIncrement);

        struct Range {
            int lower;
            int upper;
        };
        // Calculate the expected range
        Range expectedRange = {
            baseY2 - steps * yIncrement,
            baseY1 - steps * yIncrement
        };

        // Check if contentYPos is within the expected range
        return (contentYPos >= expectedRange.lower && contentYPos <= expectedRange.upper);
    }

    static int getSnappedYPosition(float contentYPos, int baseY = 380) {
        const int yIncrement = 35;
        int steps = std::round((baseY - contentYPos) / yIncrement);
        return baseY - steps * yIncrement;
    }

    static void forceFixPrio(cocos2d::CCNode* obj, int amount = 2) {
        auto oldThis = obj;
        if (auto delegate = geode::cast::typeinfo_cast<cocos2d::CCTouchDelegate*>(obj)) {
            if (auto handler = cocos2d::CCTouchDispatcher::get()->findHandler(delegate)) {
                geode::Loader::get()->queueInMainThread([obj, handler, delegate, oldThis, amount]() {
                    if (oldThis != nullptr && handler != nullptr && delegate != nullptr) {
                        if (auto dispatcher = cocos2d::CCTouchDispatcher::get()) {
                            dispatcher->setPriority(handler->m_nPriority - amount, delegate);
                        }
                    }
                });
            }
        }
    }

    static std::string replaceAll(std::string str, const std::string& from, const std::string& to) {
        size_t start_pos = 0;
        while((start_pos = str.find(from, start_pos)) != std::string::npos) {
            str.replace(start_pos, from.length(), to);
            start_pos += to.length(); // Handles case where 'to' is a substring of 'from'
        }
        return str;
    }

    static cocos2d::CCSprite* roleIDToSprite(int role, float scale) {
        cocos2d::CCSprite* spr;
        switch (role) {
            case 0: // default
            default:
                spr = cocos2d::CCSprite::createWithSpriteFrameName("GJ_profileButton_001.png");
                break;
            case 1: // verified
                spr = cocos2d::CCSprite::createWithSpriteFrameName("GJ_editBtn_001.png");
                scale -= 0.18F;
                break;
            case 2: // moderator
                spr = cocos2d::CCSprite::createWithSpriteFrameName("GJ_starBtn_001.png");
                scale += 0.1F;
                break;
            case 3: // admin
                spr = cocos2d::CCSprite::createWithSpriteFrameName("GJ_starBtnMod_001.png");
                scale += 0.1F;
                break;
        }
        spr->setScale(scale);
        return spr;
    }
};
