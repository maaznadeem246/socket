module;
#include "../core/application.hh"

/**
 * @module ssc.application
 * @description Core platform agnostic WebView APIs
 */
export module ssc.application;
import ssc.runtime;

using ssc::runtime::Runtime;

export namespace ssc::application {
  using ssc::core::application::CoreApplication;
  class Application : public CoreApplication {
    public:
      Runtime runtime;

      Application (const Application&) = delete;
      Application () : CoreApplication() {}
    #if defined(_WIN32)
      Application (void* hInstance) : CoreApplication(hInstance) {}
    #else
      Application (int unused) : CoreApplication() {}
    #endif
  };
}
