#ifndef SSC_DESKTOP_APPLICATION_H
#define SSC_DESKTOP_APPLICATION_H

#include <socket/socket.hh>

// namespace log = ssc::runtime::log;

namespace ssc::desktop {
  using runtime::Runtime;
  using runtime::Window;
  using runtime::WindowManager;
  using runtime::WindowOptions;
  using runtime::CoreApplication;

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

        this->dispatch([this] () mutable {
          this->onResume();
        });

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
        // TODO
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

#endif
