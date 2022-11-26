#ifndef SSC_SOCKET_H
#define SSC_SOCKET_H

#include "common.hh"
#include "config.hh"
#include "env.hh"
#include "json.hh"
#include "log.hh"
#include "platform.hh"
#include "runtime.hh"
#include "utils.hh"
#include "version.hh"
#include "webview.hh"
#include "window.hh"

#if defined(__APPLE__)
  #if !TARGET_OS_IPHONE && !TARGET_IPHONE_SIMULATOR
    #include "process.hh"
  #endif
#endif

namespace ssc {
  using namespace version;

  using config::Config;
  using runtime::CoreApplication; // NOLINT
  using runtime::Process;
  using runtime::Runtime; // NOLINT
  using runtime::Window;
  using runtime::WindowOptions;
  using runtime::WindowManager;
  using runtime::WindowManagerOptions;

  /**
   * Initializes various library globals, one time
   */
  extern void init (Config& config, const int argc, const char** argv);

  #if defined(_WIN32)
    extern FILE* console;
  #endif
}
#endif
