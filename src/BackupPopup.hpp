#pragma once

#include <Geode/Geode.hpp>
#include <dankmeme.argon/argon.hpp>

using namespace geode::prelude;

class BackupPopup : public Popup, public UploadPopupDelegate {
public:
    static BackupPopup* create();
    void onShowNotice(CCObject*);
    static void showNotice();
    void refreshStatus();
    void onClosePopup(UploadActionPopup* popup) override;

protected:
    bool init() override;

private:
    void onSave(CCObject*);
    void onSaveLocalLevels(CCObject*);
    void onLoad(CCObject*);
    void onLoadLocalLevels(CCObject*);
    void onDelete(CCObject*);
    void onModSettings(CCObject*);
    void onMembershipPopup(CCObject*);

    geode::Label* sizeLabel = nullptr;
    geode::Label* lastSavedLabel = nullptr;
    geode::Label* freeSpaceLabel = nullptr;
    geode::Label* subscriberLabel = nullptr;

    geode::Button* m_saveAccountItem = nullptr;
    geode::Button* m_saveLevelsItem = nullptr;
    geode::Button* m_loadItem = nullptr;
    geode::Button* m_loadLevelsItem = nullptr;
    geode::Button* m_deleteItem = nullptr;

    // helper methods to update the combined size label
    void setCombinedSize(long long saveBytes, long long levelBytes);
    void setCombinedSizeNA();
    void setCombinedSizeLoading();
    void setLastSavedFromCheckResponse(const std::string& jsonStr);
    void setFreeSpaceAndTotal(int freePercentage, long long totalSize, long long maxDataSize);
    void setFreeSpaceNA();
    void fetchAndUpdateStatus();

private:
    async::TaskHolder<web::WebResponse> m_listener;
};