module;
#include "../core/application.hh"

/**
 * @module ssc.application
 * @description Core platform agnostic WebView APIs
 */
export module ssc.application;
import ssc.runtime;
import ssc.window;
import ssc.types;

using namespace ssc::types;
using ssc::runtime::Runtime;
using ssc::window::WindowManager;

export namespace ssc::application {
  using ssc::core::application::CoreApplication;
  class Application : public CoreApplication {
    public:
      WindowManager windowManager;
      AtomicBool running = false;
      Runtime runtime;

      Application (const Application&) = delete;
      Application (const int argc, const char** argv)
        : windowManager(*this, runtime),
          CoreApplication(argc, argv) {}

    #if defined(_WIN32) // Windows
      Application (void* hInstance, const int argc, const char** argv)
        : windowManager(*this, runtime),
          CoreApplication(hInstance, argc, argv) {}
    #else // POSIX
      Application (int unused, const int argc, const char** argv)
        : windowManager(*this, runtime),
          CoreApplication(unused, argc, argv) {}
    #endif

      void start () {
        this->runtime.start();
        // start the platform specific event loop for the main
        // thread and run it until it returns a non-zero int.
        this->running = true;
        while (this->running && this->run() == 0);
      }

      void stop () {
        this->running = false; // atomic
        this->runtime.stop();
      }
  };
}
