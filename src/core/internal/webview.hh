#ifndef SSC_CORE_INTERNAL_WEBVIEW_HH
#define SSC_CORE_INTERNAL_WEBVIEW_HH

#include "../private.hh"
#include "../window.hh"

#if defined(__APPLE__)
using CoreSchemeTask = id<WKURLSchemeTask>;

#if TARGET_OS_IPHONE || TARGET_IPHONE_SIMULATOR
@interface CoreWKWebView : WKWebView
@end
#else
@interface CoreWKWebView : WKWebView<
  WKUIDelegate,
  NSDraggingDestination,
  NSFilePromiseProviderDelegate,
  NSDraggingSource
>
-     (NSDragOperation) draggingSession: (NSDraggingSession*) session
  sourceOperationMaskForDraggingContext: (NSDraggingContext) context;
@end
#endif

@interface CoreSchemeHandler : NSObject<WKURLSchemeHandler>
@property (nonatomic) ssc::core::webview::CoreSchemeHandler* handler;
@property (nonatomic) ssc::core::webview::CoreSchemeTaskManager<CoreSchemeTask>* taskManager;
- (void) webView: (CoreWKWebView*) webview startURLSchemeTask: (CoreSchemeTask) task;
- (void) webView: (CoreWKWebView*) webview stopURLSchemeTask: (CoreSchemeTask) task;
@end

@interface CoreNavigationDelegate : NSObject<WKNavigationDelegate>
-                  (void) webview: (CoreWKWebView*) webview
  decidePolicyForNavigationAction: (WKNavigationAction*) navigationAction
                  decisionHandler: (void (^)(WKNavigationActionPolicy)) decisionHandler;
@end
#endif

namespace ssc::core::window {
  // forward
  class Window;
}

namespace ssc::core::webview {
  using ssc::core::window::CoreWindow;

  class CoreWebViewInternals {
    public:
      CoreWebView* coreWebView = nullptr;
      CoreWindow* coreWindow = nullptr;
      CoreIPCSchemeHandler* coreIPCSchemeHandler = nullptr;
    #if defined(__APPLE__)
      CoreWKWebView* webview = nullptr; // aka WKWebView
      CoreNavigationDelegate* navigationDelegate = nullptr;
    #endif
      CoreWebViewInternals (
        CoreWebView* coreWebView,
        CoreWindow* coreWindow,
        CoreDataManager* coreDataManager,
        CoreIPCSchemeRequestRouteCallback onIPCSchemeRequestRouteCallback,
        const javascript::Script preloadScript
      );

      ~CoreWebViewInternals ();
  };
}
#endif
