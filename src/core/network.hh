#ifndef SSC_CORE_NETWORK_HH
#define SSC_CORE_NETWORK_HH

#include <socket/platform.hh>
#include "types.hh"

namespace ssc::core::network {
  using namespace types;

  using NetworkStatusChangeCallback = std::function<void(
    const String& statusName,
    const String& statusMessage
  )>;

  class NetworkStatusObserver {
    NetworkStatusChangeCallback onNetworkStatusChangeCallback;
    void *internal = nullptr;
    public:
      NetworkStatusObserver () = default;
      NetworkStatusObserver (NetworkStatusChangeCallback callback);
      ~NetworkStatusObserver ();
      void onNetworkStatusChange (
        const String& statusName,
        const String& statusMessage
      ) const;
  };
#if !defined(SSC_INLINE_INCLUDE)
}
#endif
#endif
