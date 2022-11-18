#ifndef SSC_CORE_APPLICATION_H
#define SSC_CORE_APPLICATION_H

#include "config.hh"
#include "data.hh"
#include "string.hh"
#include "types.hh"

namespace ssc::core::window {
  // forward
  class CoreWindow;
}

namespace ssc::core::application {
  using namespace string;
  using namespace types;
  using config::Config;

  struct Callbacks {
    ExitCallback onExit = nullptr;
  };

  class CoreApplicationInternals;
  class CoreApplication {
    public:
      static inline AtomicBool isReady = false;
      static inline CoreApplication* instance = nullptr;

      static CoreApplication* getInstance () {
        return CoreApplication::instance;
      }

      CoreApplicationInternals* internals = nullptr;
      AtomicBool exitWasRequested = false;
      AtomicBool wasStartedFromCli = false;
      AtomicBool running = false;
      AtomicBool started = false;
      Callbacks callbacks;
      Config config;
      const int argc;
      const char** argv;

      CoreApplication (const int argc, const char** argv);
      ~CoreApplication ();

      #if defined(_WIN32)
        void* hInstance;
        CoreApplication (
          /* HINSTANCE*/ void* hInstance,
          const int argc,
          const char** argv
        );
      #else
        CoreApplication (
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

      virtual ssc::core::window::CoreWindow* createDefaultWindow () = 0;
      virtual ssc::core::window::CoreWindow* createWindow () = 0;
      virtual void start () = 0;
      virtual void stop () = 0;
      virtual void onPause () = 0;
      virtual void onResume () = 0;
  };
}
#endif
