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
import ssc.uv;

using namespace ssc::types;
using ssc::runtime::Runtime;
using ssc::window::Window;
using ssc::window::WindowManager;
using ssc::window::WindowOptions;

export namespace ssc::application {
  using ssc::core::application::CoreApplication;
  class Application : public CoreApplication {
    public:
      WindowManager windowManager;
      Runtime runtime;

      Application (const Application&) = delete;
      Application () : Application(0, nullptr) {}
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

      Window* createDefaultWindow () {
        return this->windowManager.createDefaultWindow(WindowOptions {
          .cwd = this->cwd(),
          .config = this->config
        });
      }

      Window* createWindow () {
        return this->windowManager.createWindow(WindowOptions {
          .cwd = this->cwd(),
          .config = this->config
        });
      }

      void start () {
        auto cwd = this->cwd();
        uv_chdir(cwd.c_str());

        this->started = true;
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

      void onPause () {
        // TODO(@jwerle)
      }

      void onResume () {
        for (auto window : this->windowManager.windows) {
          if (window != nullptr) {
            window->bridge.bluetooth.startScanning();
          }
        }
      }
  };
}
