#include "DescPopup.hpp"
#include <Geode/ui/MDTextArea.hpp>

bool DescPopup::setup(ObjectData obj) {
    this->setTitle("Description");
    auto textArea = MDTextArea::create(obj.description, {300, 130});
    m_mainLayer->addChildAtPosition(textArea, Anchor::Center);
    return true;
}
