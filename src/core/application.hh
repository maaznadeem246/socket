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
  using data::CoreDataManager;

  struct Callbacks {
    ExitCallback onExit = nullptr;
  };

  class CoreApplication {
    // an opaque pointer to the configured `WindowFactory<Window, App>`
    void *windowFactory = nullptr;
    public:
      static inline AtomicBool isReady = false;

      AtomicBool exitWasRequested = false;
      // FIXME(jwerle): use 'SSC_CLI' env var instead of `wasStartedFromCli` boolean
      AtomicBool wasStartedFromCli = false;
      Callbacks callbacks;
      CoreDataManager dataManager;
      Config appData; // FIXME(jwerle): use 'config` instead of `appData`
      Config config;

      CoreApplication ();

      #if defined(_WIN32)
        void* hInstance;
        CoreApplication (/* HINSTANCE*/ void* hInstance);
      #else
        CoreApplication (int unused);
      #endif

      int run ();
      void kill ();
      void exit (int code);
      void restart ();
      void dispatch (std::function<void()>);
      String getCwd ();

      void setWindowFactory (void *windowFactory) {
        this->windowFactory = windowFactory;
      }

      void * getWindowFactory () const {
        return this->windowFactory;
      }
  };

  // FIXME(jwerle): move this to globa headers
#if defined(_WIN32)
  extern FILE* console;
#endif
}
#endif
