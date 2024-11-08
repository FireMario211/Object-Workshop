#pragma once
#include <Geode/Geode.hpp>

using namespace geode::prelude;

enum ObjectStatus {
    PENDING = 0,
    LISTED = 1,
    UNLISTED = 2,
    BANNED = 3
};

struct ReportData {
    int accountID;
    std::string reason;
    //std::string timestamp;
};

struct ObjectData {
    int id;
    std::string name;
    std::string description;
    std::string authorName;
    double rating = 0;
    int authorAccId;
    int downloads = 0;
    int favorites = 0;
    int ratingCount = 0;
    std::string objectString;
    bool favorited = false;
    std::unordered_set<std::string> tags;
    ObjectStatus status;

    int featured = 0;

    std::string created;
    std::string updated;
    int version = 1;

    int commentPage = 1;
    int maxCommentPage = 1;

    std::vector<ReportData> reports;
};

class ObjectItem : public CCScale9Sprite {
    protected:
        CCClippingNode* m_clippingNode;
        ObjectData m_data;
        virtual bool init(LevelEditorLayer*, ObjectData);
        CCScale9Sprite* bgSpr;
    public:
        ObjectData getData() { return m_data; };
        static CCNode* createStars(double rating);
        static CCMenu* createClickableStars(CCObject* target, SEL_MenuHandler callback);
        static ccColor3B starColor(double rating);
        static ObjectItem* create(LevelEditorLayer*, ObjectData);
};
