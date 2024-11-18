// TODO: Remove after fig accepts my pr or updates to v4!

#pragma once

#include <Geode/Geode.hpp>
#include <Geode/utils/web.hpp>

using namespace geode::prelude;

namespace authentication {
    class AuthenticationManager {
    private:
        std::string m_token = "-1";
        EventListener<web::WebTask> m_listener;

    public:
        static AuthenticationManager* get() {
            static AuthenticationManager* instance = new AuthenticationManager;
            return instance;
        }

        void getAuthenticationToken(std::function<void(std::string)> callback, std::function<void(std::string)> onFailure = [](std::string){});
    };
}
