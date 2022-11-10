#ifndef SSC_CORE_WEBVIEW_HH
#if !defined(SSC_INLINE_INCLUDE)
#define SSC_CORE_WEBVIEW_HH

#include "internal/webview.hh"
#include "platform.hh"
#include "headers.hh"
#include "types.hh"
#include "string.hh"
#include "json.hh"
#include "ipc/data.hh"
#include "ipc/message.hh"
#endif

#if !defined(SSC_INLINE_INCLUDE)
namespace ssc::webview {
  using namespace ssc::types;
  using ssc::ipc::data::DataManager;
  using ssc::ipc::message::Message;
  using ssc::string::String;
#endif
  // forward
  struct SchemeRequest;

  using InternalDataManager = internal::webview::DataManager;

  using SchemeResponseStatusCode = internal::webview::SchemeResponseStatusCode;
  using SchemeResponseHeaders = internal::webview::SchemeResponseHeaders;
  using SchemeRequestHeaders = internal::webview::SchemeRequestHeaders;

  struct SchemeResponseBody : public internal::webview::SchemeResponseBody {};
  struct SchemeRequestBody : public internal::webview::SchemeRequestBody {};
  struct SchemeResponse : public internal::webview::SchemeResponse {};
  struct SchemeRequest : public internal::webview::SchemeRequest {};

  using SchemeRequestCallback = internal::webview::SchemeRequestCallback;
  using IPCSchemeRequestRouteCallback = internal::webview::IPCSchemeRequestRouteCallback;

  class SchemeHandler : public internal::webview::SchemeHandler {
    public:
      SchemeHandler () = default;
      SchemeHandler (
        const String& scheme,
        DataManager* dataManager
      ) : SchemeHandler(scheme, dataManager, nullptr) {
        // noop
      }

      SchemeHandler (
        const String& scheme,
        DataManager* dataManager,
        SchemeRequestCallback onSchemeRequestCallback
      ) : internal::webview::SchemeHandler(
        scheme,
        (InternalDataManager*) dataManager,
        onSchemeRequestCallback
      ) {
        // noop
      }
  };

  class IPCSchemeHandler : public internal::webview::IPCSchemeHandler {
    public:
      IPCSchemeHandler () = default;
      IPCSchemeHandler (
        DataManager* dataManager,
        IPCSchemeRequestRouteCallback onIPCSchemeRequestRouteCallback
      ) : internal::webview::IPCSchemeHandler(
        (InternalDataManager*) dataManager,
        onIPCSchemeRequestRouteCallback
      ) {
        // noop
      }
  };
#if !defined(SSC_INLINE_INCLUDE)
}
#endif
#endif
