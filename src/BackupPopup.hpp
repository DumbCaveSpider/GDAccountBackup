#pragma once

#include "Geode/cocos/menu_nodes/CCMenuItem.h"
#include <Geode/Geode.hpp>
#include <argon/argon.hpp>


using namespace geode::prelude;


class BackupPopup : public Popup, public UploadPopupDelegate {
public:
  static BackupPopup *create();
  void onShowNotice(CCObject *);
  static void showNotice();
  void refreshStatus();
  void onClosePopup(UploadActionPopup* popup) override;

protected:
  bool init() override;

private:
  void onSave(CCObject *);
  void onSaveLocalLevels(CCObject *);
  void onLoad(CCObject *);
  void onLoadLocalLevels(CCObject *);
  void onDelete(CCObject *);
  void onModSettings(CCObject *);
  void onMembershipPopup(CCObject *);

  CCLabelBMFont *sizeLabel = nullptr;
  CCLabelBMFont *lastSavedLabel = nullptr;
  CCLabelBMFont *freeSpaceLabel = nullptr;
  CCLabelBMFont *subscriberLabel = nullptr;

  CCMenuItemSpriteExtra *m_saveAccountItem = nullptr;
  CCMenuItemSpriteExtra *m_saveLevelsItem = nullptr;
  CCMenuItemSpriteExtra *m_loadItem = nullptr;
  CCMenuItemSpriteExtra *m_loadLevelsItem = nullptr;
  CCMenuItemSpriteExtra *m_deleteItem = nullptr;

  // helper methods to update the combined size label
  void setCombinedSize(long long saveBytes, long long levelBytes);
  void setCombinedSizeNA();
  void setCombinedSizeLoading();
  void setLastSavedFromCheckResponse(const std::string &jsonStr);
  void setFreeSpaceAndTotal(int freePercentage, long long totalSize,
                            long long maxDataSize);
  void setFreeSpaceNA();
  void fetchAndUpdateStatus();

private:
  async::TaskHolder<web::WebResponse> m_listener;
};