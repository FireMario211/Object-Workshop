#pragma once
#include <Geode/Geode.hpp>
#include "../utils.hpp"

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

template<>
struct matjson::Serialize<ReportData> {
    static Result<ReportData> fromJson(matjson::Value const& value) {
        ReportData data;

        GEODE_UNWRAP_INTO(data.accountID, value["account_id"].asInt());
        GEODE_UNWRAP_INTO(data.reason, value["reason"].asString());
        return Ok(data);
    }
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

template<>
struct matjson::Serialize<ObjectData> {
    static Result<ObjectData> fromJson(matjson::Value const& value) {
        ObjectData data;

        GEODE_UNWRAP_INTO(data.id, value["id"].asInt());
        GEODE_UNWRAP_INTO(data.name, value["name"].asString());
        GEODE_UNWRAP_INTO(data.description, value["description"].asString());
        GEODE_UNWRAP_INTO(data.authorName, value["account_name"].asString());
        GEODE_UNWRAP_INTO(data.rating, value["rating"].asDouble());
        GEODE_UNWRAP_INTO(data.authorAccId, value["account_id"].asInt());
        GEODE_UNWRAP_INTO(data.downloads, value["downloads"].asInt());
        GEODE_UNWRAP_INTO(data.favorites, value["favorites"].asInt());
        GEODE_UNWRAP_INTO(data.ratingCount, value["rating_count"].asInt());
        GEODE_UNWRAP_INTO(data.objectString, value["data"].asString());
        GEODE_UNWRAP_INTO(auto o_tags, value["tags"].asArray());
        GEODE_UNWRAP_INTO(auto o_status, value["status"].asInt());
        GEODE_UNWRAP_INTO(data.created, value["created"].asString());
        GEODE_UNWRAP_INTO(data.updated, value["updated"].asString());
        GEODE_UNWRAP_INTO(data.version, value["version"].asInt());
        GEODE_UNWRAP_INTO(data.featured, value["featured"].asInt());
        data.tags = Utils::arrayToUnorderedSet<std::string>(o_tags);
        data.status = static_cast<ObjectStatus>(o_status);
        return Ok(data);
    }
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
