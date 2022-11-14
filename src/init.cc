#include <socket/socket.hh>

namespace ssc {
  using config::GlobalConfig;
  static bool initialized = false;
  void init (GlobalConfig& config) {
    if (initialized) return;
    initialized = true;
    ssc::config::init(config);

    if (ssc::config::isDebugEnabled()) {
    #if defined(_WIN32)
      AllocConsole();
      freopen_s(&console, "CONOUT$", "w", stdout);
    #endif

    #if defined(__linux__)
      gtk_init_check(0, nullptr);
    #endif
    }
  }
}
