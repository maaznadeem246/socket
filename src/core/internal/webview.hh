#ifndef SSC_CORE_INTERNAL_WEBVIEW_HH
#define SSC_CORE_INTERNAL_WEBVIEW_HH

#if !defined(SSC_INLINE_INCLUDE)
#include "../platform.hh"
#endif

namespace ssc::internal::webview {
  #define SSC_INLINE_INCLUDE
  #include "../types.hh"
  #include "../string.hh"
  #include "../utils.hh"
  namespace JSON {
    #include "../json.hh"
  }
  namespace javascript {
    #include "../javascript.hh"
  }
  #include "../codec.hh"
  #include "../headers.hh"
  #include "../ipc/data.hh"
  #include "../ipc/message.hh"
  #undef SSC_INLINE_INCLUDE

  // forward
  struct SchemeRequest;

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
