#pragma once

#include <Geode/Geode.hpp>
#include <argon/argon.hpp>

#include "LoadingPopup.hpp"

using namespace geode::prelude;

class BackupPopup : public Popup<> {
     public:
      static BackupPopup* create();
      void onShowNotice(CCObject*);
      static void showNotice();

     protected:
      bool setup() override;

     private:
      void onSave(CCObject*);
      void onSaveLocalLevels(CCObject*);
      void onLoad(CCObject*);
      void onLoadLocalLevels(CCObject*);
      void onDelete(CCObject*);
      void onModSettings(CCObject*);

      void disableButton(CCObject* sender);
      void enableButton(CCObject* sender);

      CCLabelBMFont* sizeLabel = nullptr;
      CCLabelBMFont* lastSavedLabel = nullptr;

      // helper methods to update the combined size label
      void setCombinedSize(long long saveBytes, long long levelBytes);
      void setCombinedSizeNA();
      void setCombinedSizeLoading();
      void setLastSavedFromCheckResponse(const std::string& jsonStr);
      // Loading overlay
      LoadingPopup* m_loadingPopup = nullptr;
      void showLoading(const std::string& msg);

      void hideLoading();
};