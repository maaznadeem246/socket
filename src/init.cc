#include <socket/socket.hh>
#include <span>

#include "core/config.hh"
#include "core/types.hh"

using namespace ssc::core::types;

namespace ssc {
  using config::GlobalConfig;
  static AtomicBool isInitialized = false;
  void init (
    GlobalConfig& config,
    const int argc,
    const char** argv
  ) {
    if (isInitialized) {
      return;
    }

    isInitialized = true;
    ssc::config::init(config);

    if (ssc::config::isDebugEnabled()) {
      #if defined(_WIN32)
        AllocConsole();
        freopen_s(&console, "CONOUT$", "w", stdout);
      #elif defined(__linux__)
        gtk_init_check(0, nullptr);
      #endif

      if (config.has("name")) {
        config["name"] += "-dev";
      }

      if (config.has("title")) {
        config["title"] += "-dev";
      }
    }

    for (auto const value : std::span(argv, argc)) {
      auto arg = String(value);
      if (arg.find("--test") == 0) {
        if (config.has("name")) {
          config["name"] += "-test";
        }

        if (config.has("title")) {
          config["title"] += "-test";
        }
      }
    }
  }
}
