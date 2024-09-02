#pragma once

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
    static std::string menuIndexToString(int menuIndex) {
        switch (menuIndex) {
            case 0:
            default:
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
                return "Trending";
            case 6:
                return "Most Recent";
            case 7:
                return "Pending";
            case 8:
                return "Reports";

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


};
