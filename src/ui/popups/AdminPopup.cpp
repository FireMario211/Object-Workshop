#include "AdminPopup.hpp"

bool AdminPopup::setup(UserData user, UserData managingUser) {
    m_user = user;
    m_managingUser = managingUser;
    this->setTitle(fmt::format("Manage {}", m_managingUser.name));
    return true;
}
