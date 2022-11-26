#ifndef SSC_RUNTIME_APPLICATION_H
#define SSC_RUNTIME_APPLICATION_H

#include <socket/socket.hh>

namespace ssc::runtime::window {
  // forward
  class Window;
}
namespace ssc::runtime::application {
  // forward
  class ApplicationInternals;

  using namespace window;

  class Application {
    public:
      static inline AtomicBool isReady = false;
      static inline Application* instance = nullptr;

      static Application* getInstance () {
        return Application::instance;
      }

      struct Callbacks {
        ExitCallback onExit = nullptr;
      };

      ApplicationInternals* internals = nullptr;
      AtomicBool exitWasRequested = false;
      AtomicBool wasStartedFromCli = false;
      AtomicBool running = false;
      AtomicBool started = false;
      Callbacks callbacks;
      //Runtime runtime;
      Config config;
      const int argc;
      const char** argv;

      Application (const Application&) = delete;
      Application ();
      Application (const int argc, const char** argv);
      ~Application ();

      #if defined(_WIN32)
        void* hInstance;
        Application (
          /* HINSTANCE*/ void* hInstance,
          const int argc,
          const char** argv
        );
      #else
        Application (
          int unused,
          const int argc,
          const char** argv
        );
      #endif

      int run ();
      void kill ();
      void exit (int code);
      void restart ();
      void dispatch (Function<void()>);
      String cwd ();

      virtual Window* createDefaultWindow () = 0;
      virtual Window* createWindow () = 0;
      virtual void start () = 0;
      virtual void stop () = 0;
      virtual void onPause () = 0;
      virtual void onResume () = 0;
  };
}

#if defined(__APPLE__)
  #if TARGET_OS_IPHONE || TARGET_IPHONE_SIMULATOR
    @interface ApplicationDelegate : NSObject<UIApplicationDelegate >
    @end

    @interface IOSApplication : NSObject<UIApplicationDelegate> {
      // initialized in `application:didFinishLaunchingWithOptions:` selector
      ssc::runtime::application::Application* app;
      ssc::runtime::window::Window* window;
    }
    @end
  #else
    @interface ApplicationDelegate : NSObject
    @end
  #endif
#endif

namespace ssc::runtime::application {
  class ApplicationInternals {
    public:
      Application* coreApplication;
    #if defined(__APPLE__)
      ApplicationDelegate* delegate;
    #endif

      ApplicationInternals (Application* coreApplication);
  };
}

#endif
