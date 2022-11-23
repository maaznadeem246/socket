#ifndef SSC_CORE_INTERNAL_APPLICATION_HH
#define SSC_CORE_INTERNAL_APPLICATION_HH

#include "../application.hh"
#include "../private.hh"
#include "../window.hh"

#if defined(__APPLE__)
  #if TARGET_OS_IPHONE || TARGET_IPHONE_SIMULATOR
    @interface CoreApplicationDelegate : NSObject<UIApplicationDelegate >
    @end

    @interface IOSApplication : NSObject<UIApplicationDelegate> {
      // initialized in `application:didFinishLaunchingWithOptions:` selector
      ssc::core::application::CoreApplication* app;
      ssc::core::window::CoreWindow* window;
    }
    @end
  #else
    @interface CoreApplicationDelegate : NSObject
    @end
  #endif
#endif

namespace ssc::core::application {
  class CoreApplicationInternals {
    public:
      CoreApplication* coreApplication;
    #if defined(__APPLE__)
      CoreApplicationDelegate* delegate;
    #endif

      CoreApplicationInternals (CoreApplication* coreApplication);
  };
}

#endif
