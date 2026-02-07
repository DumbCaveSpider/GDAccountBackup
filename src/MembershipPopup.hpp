#include <Geode/Geode.hpp>

using namespace geode::prelude;

class MembershipPopup : public Popup {
     public:
      static MembershipPopup* create();

     protected:
      bool init() override;
      void onKofi(CCObject* sender);
      void onApplyMembership(CCObject* sender);

     private:
      TextInput* m_emailInput = nullptr;
      async::TaskHolder<web::WebResponse> m_listener;
};