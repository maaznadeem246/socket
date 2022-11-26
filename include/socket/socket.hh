#ifndef SSC_SOCKET_H
#define SSC_SOCKET_H

#include "common.hh"
#include "config.hh"
#include "env.hh"
#include "json.hh"
#include "platform.hh"
#include "version.hh"

namespace ssc {
  using namespace version;
  using Config = config::Config;

  /**
   * Initializes various library globals, one time
   */
  extern void init (Config& config, const int argc, const char** argv);

  #if defined(_WIN32)
    extern FILE* console;
  #endif
}
#endif
