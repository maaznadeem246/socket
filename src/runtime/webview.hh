#ifndef SSC_RUNTIME_WEBVIEW_HH
#define SSC_RUNTIME_WEBVIEW_HH

#include <socket/platform.hh>

#if defined(_WIN32)
  #include <WebView2.h>
  #include <WebView2Experimental.h>
  #include <WebView2EnvironmentOptions.h>
  #include <WebView2ExperimentalEnvironmentOptions.h>
  #pragma comment(lib, "advapi32.lib")
  #pragma comment(lib, "shell32.lib")
  #pragma comment(lib, "version.lib")
  #pragma comment(lib, "user32.lib")
  #pragma comment(lib, "WebView2LoaderStatic.lib")
  #pragma comment(lib, "uv_a.lib")
#endif

#include "ipc.hh"
#include "runtime.hh"

namespace ssc::runtime::window {
  // forward
  class Window;
}

namespace ssc::runtime::webview {
  // forward
  struct SchemeRequest;
  class WebViewInternals;

  using ssc::runtime::window::Window;
  using namespace ipc;

  using SchemeResponseStatusCode = unsigned int;
  using SchemeResponseHeaders = Headers;
  using SchemeRequestHeaders = Headers;

  struct SchemeResponseBody {
    JSON::Any json = nullptr;
    char* bytes = nullptr;
    size_t size = 0;
  };

  struct SchemeRequestBody {
    char* bytes = nullptr;
    size_t size = 0;
  };

  struct SchemeResponse {
    const SchemeRequest* request = nullptr;
    SchemeResponseStatusCode statusCode = 200;
    SchemeResponseHeaders headers;
    SchemeResponseBody body;

    size_t size () const {
      if (this->body.size > 0) {
        return this->body.size;
      }

      return this->body.json.str().size();
    }
  };

  class SchemeRequest {
    public:
      String method;
      String url;
      String id;
      SchemeRequestBody body;
      SchemeResponse response;
      Message message;
      void *internal = nullptr;

      SchemeRequest (const String url)
        : SchemeRequest("GET", url) { }
      SchemeRequest (const String method, const String url)
        : SchemeRequest(method, url, { nullptr, 0 }) { }

    SchemeRequest (
      const String method,
      const String url,
      SchemeRequestBody body
    ) : message(url) {
      this->method = method;
      this->url = url;
      this->body = body;
      this->response.request = this;
    }

    void end (
      const SchemeResponseStatusCode statusCode,
      const SchemeResponseHeaders headers,
      JSON::Any json
    );

    void end (
      const SchemeResponseStatusCode statusCode,
      const SchemeResponseHeaders headers,
      const char* bytes,
      size_t size
    );

    void end (
      const SchemeResponseStatusCode statusCode,
      const SchemeResponseHeaders headers,
      const SchemeResponseBody body
    );

    void end (
      const SchemeResponseStatusCode statusCode,
      const SchemeResponseHeaders headers,
      JSON::Any json,
      const char* bytes,
      size_t size
    );
  };

  using SchemeRequestCallback = Function<void(SchemeRequest&)>;
  class SchemeHandler {
    public:
      String scheme;
      SchemeRequestCallback onSchemeRequestCallback;
      DataManager* dataManager;
      void* internal = nullptr;

      SchemeHandler () = default;
      SchemeHandler (
        const String& scheme,
        DataManager* dataManager,
        SchemeRequestCallback onSchemeRequestCallback
      );

      ~SchemeHandler();
      virtual void onSchemeRequest (SchemeRequest& request) = 0;
  };

  using IPCSchemeRequestRouteCallback = Function<bool(SchemeRequest&)>;
  class IPCSchemeHandler : public SchemeHandler {
    public:
      IPCSchemeRequestRouteCallback onIPCSchemeRequestRouteCallback;
      IPCSchemeHandler () = default;
      IPCSchemeHandler (
        DataManager* dataManager,
        IPCSchemeRequestRouteCallback onIPCSchemeRequestRouteCallback
      );

      void onSchemeRequest (SchemeRequest& request);
  };

  template <class SchemeTask> class SchemeTaskManager {
    public:
      using SchemeTasks = std::map<String, SchemeTask>;
      SchemeTasks tasks;
      Mutex mutex;

      SchemeTask get (String id) {
        Lock lock(this->mutex);
        if (this->tasks.find(id) == this->tasks.end()) {
          return SchemeTask{};
        }

        return this->tasks.at(id);
      }

      bool has (String id) {
        Lock lock(this->mutex);
        return id.size() > 0 && this->tasks.find(id) != this->tasks.end();
      }

      void remove (String id) {
        Lock lock(this->mutex);
        if (this->has(id)) {
          this->tasks.erase(id);
        }
      }

      void put (String id, SchemeTask task) {
        Lock lock(this->mutex);
        this->tasks.insert_or_assign(id, task);
      }
  };

  class WebView {
    public:
      WebViewInternals* internals = nullptr;
      Window* window = nullptr;
      DataManager* dataManager = nullptr;
      IPCSchemeHandler* ipcSchemeHandler = nullptr;

      WebView (
        Window* window,
        DataManager* dataManager,
        IPCSchemeRequestRouteCallback onIPCSchemeRequestRouteCallback,
        const Script preloadScript
      );

      ~WebView ();
  };
}

#if defined(__APPLE__)
namespace ssc::runtime::webview {
  using CoreSchemeTask = id<WKURLSchemeTask>;
}

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
@property (nonatomic) ssc::runtime::webview::SchemeHandler* handler;
@property (nonatomic) ssc::runtime::webview::SchemeTaskManager<ssc::runtime::webview::CoreSchemeTask>* taskManager;
- (void) webView: (CoreWKWebView*) webview startURLSchemeTask: (ssc::runtime::webview::CoreSchemeTask) task;
- (void) webView: (CoreWKWebView*) webview stopURLSchemeTask: (ssc::runtime::webview::CoreSchemeTask) task;
@end

@interface CoreNavigationDelegate : NSObject<WKNavigationDelegate>
-                  (void) webview: (WKWebView*) webview
  decidePolicyForNavigationAction: (WKNavigationAction*) navigationAction
                  decisionHandler: (void (^)(WKNavigationActionPolicy)) decisionHandler;
@end
#endif

namespace ssc::runtime::webview {
  class WebViewInternals {
    public:
      WebView* webview = nullptr;
      Window* window = nullptr;
    #if defined(__APPLE__)
      CoreWKWebView* coreWebView = nullptr; // aka WKWebView
      CoreNavigationDelegate* coreNavigationDelegate = nullptr;
    #endif
      WebViewInternals (
        WebView* webview,
        Window* window,
        DataManager* dataManager,
        SchemeHandler* ipcSchemeHandler,
        const Script preloadScript
      );

      ~WebViewInternals ();
  };
}
#endif
