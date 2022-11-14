#ifndef SOCKET_RUNTIME_H
#define SOCKET_RUNTIME_H
#include "common.hh"
#include "config.hh"
#include "platform.hh"

#include <new>
#include <string>

namespace ssc {
  using config::GlobalConfig;
  void init (GlobalConfig& config);

  #if defined(_WIN32)
    extern FILE* console;
  #endif
}
#endif
