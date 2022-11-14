#ifndef SSC_SOCKET_CONFIG_H
#define SSC_SOCKET_CONFIG_H
#include "common.hh"

namespace ssc::core::config {
  // forward
  class Config;
}

namespace ssc::config {
  using GlobalConfig = ssc::core::config::Config;
  bool isDebugEnabled ();
  void init (GlobalConfig& config);
  int getServerPort ();
}
#endif
