module;
#include "../core/application.hh"

/**
 * @module ssc.application
 * @description Core platform agnostic WebView APIs
 */
export module ssc.application;
import ssc.runtime;
import ssc.window;

using ssc::runtime::Runtime;
using ssc::window::WindowFactory;

export namespace ssc::application {
  using ssc::core::application::CoreApplication;
  class Application : public CoreApplication {
    public:
      WindowFactory windowFactory;
      Runtime runtime;

      Application (const Application&) = delete;
      Application ()
        : windowFactory(*this), CoreApplication() {}
    #if defined(_WIN32)
      Application (void* hInstance)
        : windowFactory(*this), CoreApplication(hInstance) {}
    #else
      Application (int unused)
        : windowFactory(*this), CoreApplication() {}
    #endif
  };
}
