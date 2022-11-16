#ifndef SSC_CORE_WEBVIEW_HH
#define SSC_CORE_WEBVIEW_HH

#include <socket/platform.hh>

#include "data.hh"
#include "headers.hh"
#include "ipc.hh"
#include "string.hh"
#include "types.hh"

namespace ssc::core::window {
  // forward
  class CoreWindow;
}

namespace ssc::core::webview {
  // forward
  struct CoreSchemeRequest;
  class CoreWebViewInternals;

  using namespace window;
  using namespace headers;
  using namespace string;
  using namespace types;
  using namespace ipc;
  using namespace data;

  using CoreSchemeResponseStatusCode = unsigned int;
  using CoreSchemeResponseHeaders = Headers;
  using CoreSchemeRequestHeaders = Headers;

  struct CoreSchemeResponseBody {
    JSON::Any json = nullptr;
    char* bytes = nullptr;
    size_t size = 0;
  };

  struct CoreSchemeRequestBody {
    char* bytes = nullptr;
    size_t size = 0;
  };

  struct CoreSchemeResponse {
    const CoreSchemeRequest* request = nullptr;
    CoreSchemeResponseStatusCode statusCode = 200;
    CoreSchemeResponseHeaders headers;
    CoreSchemeResponseBody body;

    size_t size () const {
      if (this->body.size > 0) {
        return this->body.size;
      }

      return this->body.json.str().size();
    }
  };

  class CoreSchemeRequest {
    public:
      String method;
      String url;
      String id;
      CoreSchemeRequestBody body;
      CoreSchemeResponse response;
      Message message;
      void *internal = nullptr;

      CoreSchemeRequest (const String url)
        : CoreSchemeRequest("GET", url) { }
      CoreSchemeRequest (const String method, const String url)
        : CoreSchemeRequest(method, url, { nullptr, 0 }) { }

    CoreSchemeRequest (
      const String method,
      const String url,
      CoreSchemeRequestBody body
    ) : message(url) {
      this->method = method;
      this->url = url;
      this->body = body;
      this->response.request = this;
    }

    void end (
      const CoreSchemeResponseStatusCode statusCode,
      const CoreSchemeResponseHeaders headers,
      JSON::Any json
    );

    void end (
      const CoreSchemeResponseStatusCode statusCode,
      const CoreSchemeResponseHeaders headers,
      const char* bytes,
      size_t size
    );

    void end (
      const CoreSchemeResponseStatusCode statusCode,
      const CoreSchemeResponseHeaders headers,
      const CoreSchemeResponseBody body
    );

    void end (
      const CoreSchemeResponseStatusCode statusCode,
      const CoreSchemeResponseHeaders headers,
      JSON::Any json,
      const char* bytes,
      size_t size
    );
  };

  using CoreSchemeRequestCallback = Function<void(CoreSchemeRequest&)>;
  class CoreSchemeHandler {
    public:
      String scheme;
      CoreSchemeRequestCallback onSchemeRequestCallback;
      CoreDataManager* coreDataManager;
      void* internal = nullptr;

      CoreSchemeHandler () = default;
      CoreSchemeHandler (
        const String& scheme,
        CoreDataManager* coreDataManager,
        CoreSchemeRequestCallback onSchemeRequestCallback
      );

      ~CoreSchemeHandler();
      virtual void onSchemeRequest (CoreSchemeRequest& request) = 0;
  };

  using CoreIPCSchemeRequestRouteCallback = Function<bool(CoreSchemeRequest&)>;
  class CoreIPCSchemeHandler : public CoreSchemeHandler {
    public:
      CoreIPCSchemeRequestRouteCallback onIPCSchemeRequestRouteCallback;
      CoreIPCSchemeHandler () = default;
      CoreIPCSchemeHandler (
        CoreDataManager* coreDataManager,
        CoreIPCSchemeRequestRouteCallback onIPCSchemeRequestRouteCallback
      );

      void onSchemeRequest (CoreSchemeRequest& request);
  };

  template <class CoreSchemeTask> class CoreSchemeTaskManager {
    public:
      using CoreSchemeTasks = std::map<String, CoreSchemeTask>;
      CoreSchemeTasks tasks;
      Mutex mutex;

      CoreSchemeTask get (String id) {
        Lock lock(this->mutex);
        if (this->tasks.find(id) == this->tasks.end()) {
          return CoreSchemeTask{};
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

      void put (String id, CoreSchemeTask task) {
        Lock lock(this->mutex);
        this->tasks.insert_or_assign(id, task);
      }
  };

  class CoreWebView {
    public:
      CoreWebViewInternals* internals = nullptr;
      CoreWindow* coreWindow = nullptr;
      CoreDataManager* coreDataManager = nullptr;
      CoreIPCSchemeHandler* ipcSchemeHandler = nullptr;

      CoreWebView (
        CoreWindow* coreWindow,
        CoreDataManager* coreDataManager,
        CoreIPCSchemeRequestRouteCallback onIPCSchemeRequestRouteCallback,
        const javascript::Script preloadScript
      );

      ~CoreWebView ();
  };
}
#endif
