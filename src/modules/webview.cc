module;
#include "../core/webview.hh"
#include <functional>

/**
 * @module ssc.webview
 * @description Core platform agnostic WebView APIs
 */
export module ssc.webview;
import ssc.string;
import ssc.ipc;

using ssc::string::String;
using ssc::ipc::data::DataManager;

export namespace ssc::webview {
  // forward
  struct SchemeRequest;

  using InternalDataManager = core::webview::DataManager;

  using SchemeResponseStatusCode = core::webview::SchemeResponseStatusCode;
  using SchemeResponseHeaders = core::webview::SchemeResponseHeaders;
  using SchemeRequestHeaders = core::webview::SchemeRequestHeaders;

  struct SchemeResponseBody : public core::webview::SchemeResponseBody {};
  struct SchemeRequestBody : public core::webview::SchemeRequestBody {};
  struct SchemeResponse : public core::webview::SchemeResponse {};
  struct SchemeRequest : public core::webview::SchemeRequest {};
  struct IPCSchemeRequest : public core::webview::IPCSchemeRequest {};

  using SchemeRequestCallback = core::webview::SchemeRequestCallback;
  using IPCSchemeRequestRouteCallback = core::webview::IPCSchemeRequestRouteCallback;

  class SchemeHandler : public core::webview::SchemeHandler {
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
      ) : core::webview::SchemeHandler(
        scheme,
        (InternalDataManager*) dataManager,
        onSchemeRequestCallback
      ) {
        // noop
      }
  };

  class IPCSchemeHandler : public core::webview::IPCSchemeHandler {
    public:
      IPCSchemeHandler () = default;
      IPCSchemeHandler (
        DataManager* dataManager,
        IPCSchemeRequestRouteCallback onIPCSchemeRequestRouteCallback
      ) : core::webview::IPCSchemeHandler(
        (InternalDataManager*) dataManager,
        onIPCSchemeRequestRouteCallback
      ) {
        // noop
      }
  };
}
