#pragma once
#include <Geode/Geode.hpp>
#include <Geode/ui/Popup.hpp>
#include "../../nodes/CommentCell.hpp"
#include <Geode/utils/web.hpp>
using namespace geode::prelude;

class VotePopup : public geode::Popup<CommentData, utils::MiniFunction<void()>> {
protected:
    EventListener<web::WebTask> m_listener;
    utils::MiniFunction<void()> m_forceRefresh;
    CommentData m_data;

    bool setup(CommentData obj, utils::MiniFunction<void()> callback) override;
    void onLike(CCObject*);
    void onDislike(CCObject*);
    void sendVote(bool like);

public:
    static VotePopup* create(CommentData obj, utils::MiniFunction<void()> callback) {
        auto ret = new VotePopup();
        if (ret->initAnchored(200.f, 115.f, obj, callback)) {
            ret->autorelease();
            return ret;
        }

        delete ret;
        return nullptr;
    }
};
