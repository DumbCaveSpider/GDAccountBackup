#pragma once

#include <Geode/Geode.hpp>
#include <argon/argon.hpp>

using namespace geode::prelude;

class BackupPopup : public Popup<> {
public:
  static BackupPopup *create();

protected:
  bool setup() override;

private:
  void onSave(CCObject *);
  void onSaveLocalLevels(CCObject *);
  void onLoad(CCObject *);
  void onLoadLocalLevels(CCObject *);
  void onDelete(CCObject *);
  void onModSettings(CCObject *);

  void disableButton(CCObject *sender);
  void enableButton(CCObject *sender);

  CCLabelBMFont *sizeLabel = nullptr;
  CCLabelBMFont *levelDataSizeLabel = nullptr;
  CCLabelBMFont *lastSavedLabel = nullptr;
  CCLabelBMFont *statusLabel = nullptr;
};