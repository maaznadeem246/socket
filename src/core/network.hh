#ifndef SSC_CORE_NETWORK_HH
#define SSC_CORE_NETWORK_HH

#include "types-headers.hh"

namespace ssc::core::network {
  using String = std::string;
  using NetworkStatusChangeCallback = std::function<void(const String&, const String&)>;

  class NetworkStatusObserver {
    NetworkStatusChangeCallback onNetworkStatusChangeCallback;
    void *internal = nullptr;
    public:
      NetworkStatusObserver () = default;
      NetworkStatusObserver (NetworkStatusChangeCallback callback);
      ~NetworkStatusObserver ();
      void onNetworkStatusChange (
        const String& statusName,
        const String& message
      ) const;
  };
}
#endif
