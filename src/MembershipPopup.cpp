#include "MembershipPopup.hpp"

#include <Geode/Geode.hpp>

#include "BackupPopup.hpp"

using namespace geode::prelude;

MembershipPopup* MembershipPopup::create() {
      auto ret = new MembershipPopup();
      if (ret && ret->initAnchored(350.f, 275.f, "GJ_square04.png")) {
            ret->autorelease();
            return ret;
      }
      CC_SAFE_DELETE(ret);
      return nullptr;
}

bool MembershipPopup::setup() {
      setTitle("Account Backup Extra Membership");

      // text input
      auto emailInput = TextInput::create(200.f, "Enter your email...", "bigFont.fnt");
      emailInput->setCommonFilter(CommonFilter::Any);
      emailInput->setPosition({m_mainLayer->getContentSize().width / 2 - 50.f,
                               m_mainLayer->getContentSize().height - 60.f});

      // apply button
      auto applyBtn = ButtonSprite::create("Apply", 80.f, true, "goldFont.fnt", "GJ_button_01.png", 0.f, 1.f);
      auto applyItem = CCMenuItemSpriteExtra::create(
          applyBtn, this, menu_selector(MembershipPopup::onApplyMembership));
      applyItem->setPosition({emailInput->getPositionX() + 150.f,
                              emailInput->getPositionY()});
      m_buttonMenu->addChild(applyItem);
      auto infoMD = MDTextArea::create(
          "By <cp>subscribing</c> to the <cg>Account Backup Extra Membership</c>, you get <cy>512MB</c> total backup storage space!\n\n"
          "To get the extra storage, please enter your <cg>email address</c> associated with your <cp>Ko-fi account</c> and click <cg>Apply</c>.\n\n"
          "If you have any issues during this process, please contact <cf>ArcticWoof</c> on [Discord](https://discord.gg/gXcppxTNxC) or [GD DMs](user:7689052).",
          CCSize(300.f, 150.f));
      infoMD->setPosition({m_mainLayer->getContentSize().width / 2,
                           m_mainLayer->getContentSize().height / 2 - 20.f});
      m_mainLayer->addChild(infoMD);
      m_emailInput = emailInput;
      m_mainLayer->addChild(emailInput);

      // kofi button
      auto kofiBtn = ButtonSprite::create("Subscribe", "goldFont.fnt", "GJ_button_03.png");
      auto kofiItem = CCMenuItemSpriteExtra::create(
          kofiBtn, this, menu_selector(MembershipPopup::onKofi));
      kofiItem->setPosition({m_mainLayer->getContentSize().width / 2, 23.f});
      m_buttonMenu->addChild(kofiItem);

      return true;
}

void MembershipPopup::onKofi(CCObject* sender) {
      utils::web::openLinkInBrowser("https://ko-fi.com/arcticwoof");
}

void MembershipPopup::onApplyMembership(CCObject* sender) {
      std::string email;
      if (m_emailInput) {
            auto txt = m_emailInput->getString();
            std::string tmp = txt;
            if (!tmp.empty()) email = tmp;
      }

      int accountId = 0;
      if (auto acct = GJAccountManager::get()) {
            accountId = acct->m_accountID;
      }

      std::string token = Mod::get()->getSavedValue<std::string>("argonToken");

      matjson::Value body = matjson::makeObject(
          {{"email", email}, {"accountId", accountId}, {"argonToken", token}});

      std::string backupUrl = Mod::get()->getSettingValue<std::string>("backup-url");

      auto req = web::WebRequest()
                     .timeout(std::chrono::seconds(30))
                     .header("Content-Type", "application/json")
                     .bodyJSON(body)
                     .post(backupUrl + "/membership");

      static geode::EventListener<web::WebTask> listener;
      listener.bind([this](web::WebTask::Event* e) {
            if (auto* resp = e->getValue()) {
                  if (resp->ok()) {
                        Notification::create("Subscribed Successfully!", NotificationIcon::Success)
                            ->show();
                        this->removeFromParentAndCleanup(true);

                        // try to find a visible BackupPopup and refresh it
                        if (auto scene = CCDirector::sharedDirector()->getRunningScene()) {
                              if (auto children = scene->getChildren()) {
                                    for (unsigned int i = 0; i < children->count(); ++i) {
                                          auto child = children->objectAtIndex(i);
                                          if (auto backup = typeinfo_cast<BackupPopup*>(child)) {
                                                backup->refreshStatus();
                                          }
                                    }
                              }
                        }
                  } else {
                        auto errorBody = resp->string();
                        std::string errorMsg = "Unknown error";
                        if (errorBody) {
                              auto parsed = matjson::Value::parse(errorBody.unwrap());
                              if (parsed) {
                                    auto obj = parsed.unwrap();
                                    if (auto err = obj["error"].asString()) {
                                          errorMsg = err.unwrap();
                                    }
                              }
                        }
                        Notification::create("Membership failed: " + errorMsg, NotificationIcon::Error)
                            ->show();
                  }
            } else if (e->isCancelled()) {
                  Notification::create("Membership request was cancelled", NotificationIcon::Error)
                      ->show();
            }
      });
      listener.setFilter(std::move(req));
}