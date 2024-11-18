#pragma once
#include <Geode/Geode.hpp>
#include "../ui/ObjectWorkshop.hpp"
#include "ObjectItem.hpp"

using namespace geode::prelude;

struct CommentData {
    int id;
    int objectID;
    std::string accountName;
    int accountID;
    std::string timestamp;
    std::string content;
    int likes;
    bool pinned;
    std::array<int, 5> icon;
    int role;
};
template<>
struct matjson::Serialize<CommentData> {
    static Result<CommentData> fromJson(matjson::Value const& value) {
        CommentData data;

        GEODE_UNWRAP_INTO(data.id, value["id"].asInt());
        GEODE_UNWRAP_INTO(data.objectID, value["object_id"].asInt());
        GEODE_UNWRAP_INTO(data.accountName, value["account_name"].asString());
        GEODE_UNWRAP_INTO(data.accountID, value["account_id"].asInt());
        GEODE_UNWRAP_INTO(data.likes, value["likes"].asInt());
        GEODE_UNWRAP_INTO(data.content, value["content"].asString());
        GEODE_UNWRAP_INTO(data.timestamp, value["timestamp"].asString());
        GEODE_UNWRAP_INTO(data.pinned, value["pinned"].asBool());
        GEODE_UNWRAP_INTO(data.role, value["role"].asInt());
        auto o_icon = value.get("icon");
        if (o_icon.isOk()) {
            auto icon_array = o_icon.unwrap().asArray().unwrapOr(std::vector<matjson::Value> {1,1,1,1,1});
            if (icon_array.size() == 5) {
                data.icon[0] = icon_array[0].asInt().unwrapOrDefault();
                data.icon[1] = icon_array[1].asInt().unwrapOrDefault();
                data.icon[2] = icon_array[2].asInt().unwrapOrDefault();
                data.icon[3] = icon_array[3].asInt().unwrapOrDefault();
                data.icon[4] = icon_array[4].asInt().unwrapOrDefault();
            }
        }
        return Ok(data);
    }
};

class OWCommentCell : public CCScale9Sprite {
    protected:
        EventListener<web::WebTask> m_listener;
        std::function<void()> m_forceRefresh;

        CommentData m_data;
        UserData m_user;
        virtual bool init(CommentData, ObjectData, UserData, std::function<void()>);
        void onVote(CCObject*);
        void onPin(CCObject*);
        void onDelete(CCObject*);
    public:
        std::string getComment() { return m_data.content; };
        CommentData getData() { return m_data; };
        static OWCommentCell* create(CommentData, ObjectData, UserData, std::function<void()>);
};
