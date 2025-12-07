#pragma once

#include <Geode/Geode.hpp>
#include <Geode/ui/GeodeUI.hpp>

using namespace geode::prelude;

class LoadingPopup : public CCBlockLayer {
public:
    static LoadingPopup* create(const std::string& message);

    void setMessage(const std::string& message);
    void show();
    void hide();

protected:
    bool initWithMessage(const std::string& message);

private:
    CCLabelBMFont* m_label = nullptr;
    LoadingSpinner* m_spinner = nullptr;
    void keyBackClicked();
};
