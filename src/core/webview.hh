#ifndef SSC_CORE_WEBVIEW_HH
#define SSC_CORE_WEBVIEW_HH

#if !defined(SSC_INLINE_INCLUDE)
#include "types.hh"
#include "ipc/message.hh"
#endif

#if !defined(SSC_INLINE_INCLUDE)
namespace ssc::webview {
  using namespace ssc::types;
  using ssc::ipc::message::Message;
#endif

  struct SchemeRequest;
  struct SchemeResponse {
    SchemeRequest* request = nullptr;
    Map headers;
    int statusCode;
    struct { char *bytes; size_t size; } body;
  };

  struct SchemeRequest {
    Message message;
    const String method;
    const String url;
    struct { char *bytes; size_t size; } body;
    void *internal = nullptr;

    void end (int statusCode, Map headers, char *bytes, size_t size);
    void end (SchemeResponse& response);
  };

  using SchemeRequestCallback = std::function<void(SchemeRequest&)>;
  class SchemeHandler {
    public:
      String scheme;
      SchemeRequestCallback onSchemeRequestCallback;
      void* internal = nullptr;
      SchemeHandler (const String& scheme);
      SchemeHandler (
        const String& scheme,
        SchemeRequestCallback onSchemeRequestCallback
      );
      ~SchemeHandler ();
      void onSchemeRequest (SchemeRequest& request);
  };

  class IPCSchemeHandler : public SchemeHandler {
    public:
      IPCSchemeHandler ();
  };
#if !defined(SSC_INLINE_INCLUDE)
}
#endif
#endif
