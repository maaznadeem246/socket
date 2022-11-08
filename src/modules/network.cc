module;

#include "../core/network.hh"

export module ssc.network;

export namespace ssc::network {
  using NetworkStatusChangeCallback = ssc::core::network::NetworkStatusChangeCallback;
  using NetworkStatusObserver = ssc::core::network::NetworkStatusObserver;
}
