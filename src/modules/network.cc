module;
#include "../core/network.hh"

/**
 * @module ssc.network
 * @description Various network utilities
 * @example
 * TODO
 */
export module ssc.network;

namespace ssc::network {
  // just re-export from core
  export using NetworkStatusChangeCallback = ssc::network::NetworkStatusChangeCallback;
  export using NetworkStatusObserver = ssc::network::NetworkStatusObserver;
}
