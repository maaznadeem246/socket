#ifndef SOCKET_RUNTIME_H
#define SOCKET_RUNTIME_H
#include "common.hh"
#include "config.hh"
#include "platform.hh"

#include <new>
#include <string>

namespace ssc {
  using config::GlobalConfig;

  /**
   * Initializes various library globals, one time
   */
  void init (
    GlobalConfig& config,
    const int argc,
    const char** argv
  );

  #if defined(_WIN32)
    extern FILE* console;
  #endif
}
#endif
