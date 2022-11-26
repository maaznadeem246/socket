#ifndef SSC_IOS_APPLICATION_H
#define SSC_IOS_APPLICATION_H

#include <socket/socket.hh>
#include <socket/utils.hh>

#include "../runtime/application.hh"
#include "../runtime/ipc.hh"
#include "../runtime/process.hh"
#include "../runtime/runtime.hh"
#include "../runtime/window.hh"

namespace ssc::ios {
  using runtime::Runtime;
  using runtime::window::Window;
  using runtime::window::WindowManager;
  using runtime::window::WindowOptions;
  using RuntimeApplication = runtime::application::Application;

  class Application : public RuntimeApplication {
    public:
      WindowManager windowManager;
      Runtime runtime;

      Application (const Application&) = delete;
      Application ()
        : Application(0, nullptr)
      {}

      Application (const int argc, const char** argv)
        : windowManager(*this, runtime),
          RuntimeApplication(argc, argv)
      {}

      Window* createDefaultWindow () {
        return this->windowManager.createDefaultWindow(WindowOptions {
          .cwd = this->cwd(),
          .config = this->config
        });
      }

      Window* createWindow () {
        return this->createDefaultWindow();
      }

      void start () {
        this->started = true;
        this->runtime.start();
        this->running = true;
      }

      void stop () {
        this->running = false;
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

#endif
