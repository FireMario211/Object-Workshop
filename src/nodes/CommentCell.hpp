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
    matjson::Array icon;
    int role;
};

class OWCommentCell : public CCScale9Sprite {
    protected:
        EventListener<web::WebTask> m_listener;
        utils::MiniFunction<void()> m_forceRefresh;

        CommentData m_data;
        UserData m_user;
        virtual bool init(CommentData, ObjectData, UserData, utils::MiniFunction<void()>);
        void onVote(CCObject*);
        void onPin(CCObject*);
        void onDelete(CCObject*);
    public:
        CommentData getData() { return m_data; };
        static OWCommentCell* create(CommentData, ObjectData, UserData, utils::MiniFunction<void()>);
};
