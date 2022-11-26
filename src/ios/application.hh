#ifndef SSC_IOS_APPLICATION_H
#define SSC_IOS_APPLICATION_H

#include <socket/socket.hh>

#if TARGET_OS_IPHONE || TARGET_IPHONE_SIMULATOR
@interface IOSApplication : IOSCoreApplication
@end
#endif

namespace ssc::ios {
  class Application : public CoreApplication {
    public:
      WindowManager windowManager;
      Runtime runtime;

      Application (const Application&) = delete;
      Application ()
        : Application(0, nullptr)
      {}

      Application (const int argc, const char** argv)
        : windowManager(*this, runtime),
          CoreApplication(argc, argv)
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
