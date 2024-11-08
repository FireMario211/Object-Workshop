#pragma once
#include <Geode/Geode.hpp>
#include "../ui/ObjectWorkshop.hpp"
#include "../ui/popups/WarningPopup.hpp"

using namespace geode::prelude;


class CaseCell : public CCScale9Sprite {
    protected:
        EventListener<web::WebTask> m_listener;
        UserData m_user;
        CaseData m_case;
        std::function<void()> m_forceRefresh;

        virtual bool init(UserData, CaseData, std::function<void()>);

        std::string caseTypeToString(CaseType type) {
            switch (type) {
                case CaseType::Warning: return "Warning";
                case CaseType::TCommentBan: return "Comment Ban (Temp)";
                case CaseType::TUploadBan: return "Upload Ban (Temp)";
                case CaseType::TBan: return "Account Ban (Temp)";
                case CaseType::CommentBan: return "Comment Ban (Perm)";
                case CaseType::UploadBan: return "Upload Ban (Perm)";
                case CaseType::Ban: return "Account Ban (Perm)";
            }
        }
    public:
        static CaseCell* create(UserData, CaseData, std::function<void()>);
};

