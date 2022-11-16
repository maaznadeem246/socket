#ifndef SSC_CORE_APPLICATION_H
#define SSC_CORE_APPLICATION_H

#include "config.hh"
#include "data.hh"
#include "string.hh"
#include "types.hh"

namespace ssc::core::application {
  using namespace string;
  using namespace types;
  using config::Config;

  struct Callbacks {
    ExitCallback onExit = nullptr;
  };

  class CoreApplication {
    public:
      static inline AtomicBool isReady = false;

      AtomicBool exitWasRequested = false;
      AtomicBool wasStartedFromCli = false;
      Callbacks callbacks;
      Config config;
      const int argc;
      const char** argv;

      CoreApplication (const int argc, const char** argv);

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

      void start ();
      void stop ();
      int run ();
      void kill ();
      void exit (int code);
      void restart ();
      void dispatch (Function<void()>);
      String getCwd ();
  };
}
#endif
