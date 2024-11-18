// TODO: Remove after fig accepts my pr or updates to v4!

#include <Geode/Geode.hpp>
#include <Geode/utils/web.hpp>

#include "authentication.hpp"

using namespace geode::prelude;
using namespace authentication;

void AuthenticationManager::getAuthenticationToken(std::function<void(std::string)> onSuccess, std::function<void(std::string)> onFailure) {
    if (m_token != "-1") {
        return onSuccess(m_token);
    }

    std::string url = "https://gd.figm.io/authentication/authenticate";
    web::WebRequest req = web::WebRequest();

    req.bodyString(fmt::format("accountid={}&gjp={}", GJAccountManager::get()->m_accountID, GJAccountManager::get()->m_GJP2));
    auto task = req.post(url);

    m_listener.bind([this, onSuccess, onFailure](web::WebTask::Event* e) {
        if (web::WebResponse* value = e->getValue()) {
            if (value->string().unwrapOr("-1") == "-1" || value->json().isErr()) {
                return onFailure("Authentication Failed");
            }

            m_token = value->json().unwrap().get<std::string>("sessionID").unwrapOr("");
            onSuccess(m_token);
        } else if (web::WebProgress* progress = e->getProgress()) {
            // The request is still in progress...
        } else if (e->isCancelled()) {
            onFailure("The request was cancelled");
        }
    });

    m_listener.setFilter(task);
}
