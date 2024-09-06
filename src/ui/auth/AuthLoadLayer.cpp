#include "AuthLoadLayer.hpp"

bool AuthLoadLayer::init() {
    if (!this->initWithColor({0, 0, 0, 75})) return false;
    auto winSize = cocos2d::CCDirector::sharedDirector()->getWinSize();
    this->m_mainLayer = CCLayer::create();
    this->m_buttonMenu = cocos2d::CCMenu::create();
    this->m_mainLayer->addChild(this->m_buttonMenu);
    this->addChild(this->m_mainLayer);
    this->registerWithTouchDispatcher();
    this->setTouchEnabled(true);

    m_loadingCircle = LoadingCircle::create();
    //m_loadingCircle->setPosition(winSize / 2);
    m_loadingCircle->setParentLayer(m_mainLayer);
    m_loadingCircle->show();
    return true;
}
void AuthLoadLayer::finished() {
    this->removeFromParentAndCleanup(true);
}
