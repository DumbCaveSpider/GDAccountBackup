#pragma once

#include <Geode/Geode.hpp>
#include <argon/argon.hpp>

using namespace geode::prelude;

class BackupPopup : public Popup<> {
    public:
    static BackupPopup* create();

    protected:
    bool setup() override;

    private:
    void onSave(CCObject*);
    void onLoad(CCObject*);
    void onDelete(CCObject*);
};