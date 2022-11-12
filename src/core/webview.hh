#ifndef SSC_CORE_WEBVIEW_HH
#define SSC_CORE_WEBVIEW_HH

#include <socket/platform.hh>

#include "headers.hh"
#include "ipc/data.hh"
#include "ipc/message.hh"
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
  using namespace ipc::message;
  using namespace ipc::data;

  using CoreSchemeResponseStatusCode = unsigned int;
  using CoreSchemeResponseHeaders = Headers;
  using CoreSchemeRequestHeaders = Headers;

  struct CoreSchemeResponseBody {
    JSON::Any json;
    char* bytes;
    size_t size;
  };

  struct CoreSchemeRequestBody {
    char* bytes;
    size_t size;
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

  struct CoreSchemeRequest {
    String method;
    String url;
    CoreSchemeRequestBody body;
    void *internal = nullptr;

    CoreSchemeRequest (const String url)
      : CoreSchemeRequest("GET", url) { }
    CoreSchemeRequest (const String method, const String url)
      : CoreSchemeRequest(method, url, { nullptr, 0 }) { }

    CoreSchemeRequest (
      const String method,
      const String url,
      CoreSchemeRequestBody body
    ) {
      this->method = method;
      this->url = url;
      this->body = body;
    }

    void end (const CoreSchemeResponse& response) const;
    void end (
      const CoreSchemeResponseStatusCode statusCode,
      const CoreSchemeResponseHeaders headers,
      const char* bytes,
      size_t size
    ) const;
  };

  using CoreSchemeRequestCallback = Function<void(const CoreSchemeRequest&)>;
  class CoreSchemeHandler {
    public:
      String scheme;
      CoreSchemeRequestCallback onSchemeRequestCallback;
      DataManager* dataManager;
      void* internal = nullptr;

      CoreSchemeHandler () = default;
      CoreSchemeHandler (
        const String& scheme,
        DataManager* dataManager,
        CoreSchemeRequestCallback onSchemeRequestCallback
      );

      ~CoreSchemeHandler();
      void onSchemeRequest (const CoreSchemeRequest request);
  };

  struct CoreIPCSchemeRequest : public CoreSchemeRequest {
    Message message;
    CoreIPCSchemeRequest (const String url)
      : CoreIPCSchemeRequest("GET", url) { }
    CoreIPCSchemeRequest (const String method, const String url)
      : CoreIPCSchemeRequest(method, url, { nullptr, 0 }) { }
    CoreIPCSchemeRequest (
      const String method,
      const String url,
      CoreSchemeRequestBody body
    ) : CoreSchemeRequest(method, url, body), message(url) {
    }
  };

  using CoreIPCSchemeRequestRouteCallback = Function<bool(const CoreIPCSchemeRequest&)>;
  class CoreIPCSchemeHandler : public CoreSchemeHandler {
    public:
      CoreIPCSchemeRequestRouteCallback onIPCSchemeRequestRouteCallback;
      CoreIPCSchemeHandler () = default;
      CoreIPCSchemeHandler (
        DataManager* dataManager,
        CoreIPCSchemeRequestRouteCallback onIPCSchemeRequestRouteCallback
      );

      void onSchemeRequest (const CoreIPCSchemeRequest request);
  };

  template <class CoreSchemeTask> class CoreSchemeTaskManager {
    public:
      using CoreSchemeTasks = std::map<std::string, CoreSchemeTask>;
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
      CoreIPCSchemeHandler* ipcSchemeHandler = nullptr;
      CoreWebViewInternals* internals = nullptr;
      CoreWindow* window = nullptr;
      DataManager* dataManager = nullptr;

      CoreWebView (
        CoreWindow* window,
        DataManager* dataManager,
        CoreIPCSchemeRequestRouteCallback onIPCSchemeRequestRouteCallback
      );

      ~CoreWebView ();

      bool addPreloadScript (const javascript::Script script);
  };
}
#endif