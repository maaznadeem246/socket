#ifndef SSC_CORE_INTERNAL_WINDOW_HH
#define SSC_CORE_INTERNAL_WINDOW_HH

#include "../private.hh"
#include "../window.hh"

#if defined(__APPLE__)
#if TARGET_OS_IPHONE || TARGET_IPHONE_SIMULATOR
@interface CoreWindowDelegate : NSObject
@end
#else
@interface CoreWindowDelegate : NSObject <NSWindowDelegate, WKScriptMessageHandler>
- (void) userContentController: (WKUserContentController*) userContentController
       didReceiveScriptMessage: (WKScriptMessage*) scriptMessage;
@end
#endif
#endif

namespace ssc::core::window {
  class CoreWindowInternals {
    public:
    #if !TARGET_OS_IPHONE && !TARGET_IPHONE_SIMULATOR
      NSWindow* window = nullptr;
      CoreWindowDelegate* delegate = nullptr;
    #endif

      CoreWindowInternals (
        CoreWindow* coreWindow,
        const CoreWindowOptions& opts
      );
  };
}

#endif
