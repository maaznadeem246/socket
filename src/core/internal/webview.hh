#ifndef SSC_CORE_INTERNAL_WEBVIEW_HH
#define SSC_CORE_INTERNAL_WEBVIEW_HH

#if !defined(SSC_INLINE_INCLUDE)
#include "../platform.hh"
#include "internal.hh"
#endif

namespace ssc::internal::webview {
  // forward
  struct SchemeRequest;

  using namespace headers;
  using namespace string;
  using namespace types;
  using namespace ipc::message;
  using namespace ipc::data;
  using namespace ipc;

  using SchemeResponseStatusCode = unsigned int;
  using SchemeResponseHeaders = Headers;
  using SchemeRequestHeaders = Headers;

  struct SchemeResponseBody {
    JSON::Any json;
    char* bytes;
    size_t size;
  };

  struct SchemeRequestBody {
    char* bytes;
    size_t size;
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

  struct SchemeRequest {
    Message message;
    String method;
    String url;
    SchemeRequestBody body;
    void *internal = nullptr;

    SchemeRequest (const String url) : SchemeRequest("GET", url) { }
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
    }

    void end (const SchemeResponse& response) const;
    void end (
      const SchemeResponseStatusCode statusCode,
      const Headers headers,
      const char* bytes,
      size_t size
    ) const;
  };

  using SchemeRequestCallback = std::function<void(const SchemeRequest&)>;
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
      void onSchemeRequest (const SchemeRequest request);
  };

  using IPCSchemeRequestRouteCallback = std::function<bool(const SchemeRequest&)>;
  class IPCSchemeHandler : public SchemeHandler {
    public:
      IPCSchemeRequestRouteCallback onIPCSchemeRequestRouteCallback;
      IPCSchemeHandler () = default;
      IPCSchemeHandler (
        DataManager* dataManager,
        IPCSchemeRequestRouteCallback onIPCSchemeRequestRouteCallback
      );

      void onSchemeRequest (const SchemeRequest request);
  };
}
#endif
