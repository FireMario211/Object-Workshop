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
