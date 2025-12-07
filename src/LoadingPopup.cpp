#include "LoadingPopup.hpp"

using namespace geode::prelude;

LoadingPopup* LoadingPopup::create(const std::string& message) {
      auto layer = new LoadingPopup();
      if (layer->initWithMessage(message)) {
            layer->autorelease();
            return layer;
      }
      CC_SAFE_DELETE(layer);
      return nullptr;
}

bool LoadingPopup::initWithMessage(const std::string& message) {
      if (!CCBlockLayer::init()) {
            return false;
      }
      // size to full screen
      auto winSize = CCDirector::sharedDirector()->getWinSize();
      this->setContentSize(winSize);
      this->setPosition({0.f, 0.f});

      // add a spinner at center
      m_spinner = LoadingSpinner::create(80.f);
      m_spinner->setPosition({winSize.width / 2, winSize.height / 2});
      m_spinner->setAnchorPoint({0.5f, 0.5f});
      this->addChild(m_spinner, 2);

      // add a label above spinner
      m_label = CCLabelBMFont::create(message.c_str(), "bigFont.fnt");
      m_label->setAlignment(kCCTextAlignmentCenter);
      m_label->setPosition({winSize.width / 2, winSize.height / 2 + 60});
      m_label->setScale(0.5f);
      this->addChild(m_label, 2);

      this->setKeyboardEnabled(false);

      return true;
}

void LoadingPopup::setMessage(const std::string& message) {
      if (m_label) {
            m_label->setString(message.c_str());
      }
}

void LoadingPopup::hide() {
      this->removeFromParentAndCleanup(true);
}

void LoadingPopup::keyBackClicked() {
      // do nothing to disable back button
      return;
}