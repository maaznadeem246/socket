module;
#include "../core/webview.hh"

/**
 * @module ssc.webview
 * @description Core platform agnostic WebView APIs
 */
export module ssc.webview;

export namespace ssc::webview {
  using SchemeResponseStatusCode = core::webview::CoreSchemeResponseStatusCode;
  using SchemeResponseHeaders = core::webview::CoreSchemeResponseHeaders;
  using SchemeRequestHeaders = core::webview::CoreSchemeRequestHeaders;

  using SchemeHandler = core::webview::CoreSchemeHandler;
  using SchemeRequest = core::webview::CoreSchemeRequest;
  using SchemeRequestBody = core::webview::CoreSchemeRequestBody;
  using SchemeResponse = core::webview::CoreSchemeResponse;
  using SchemeResponseBody = core::webview::CoreSchemeResponseBody;
  using SchemeRequestCallback = core::webview::CoreSchemeRequestCallback;

  using IPCSchemeHandler = core::webview::CoreIPCSchemeHandler;
  using IPCSchemeRequest = core::webview::CoreSchemeRequest;
  using IPCSchemeRequestRouteCallback = core::webview::CoreIPCSchemeRequestRouteCallback;

  using WebView = core::webview::CoreWebView;
  using WebViewInternals = core::webview::CoreWebViewInternals;

  template <class SchemeTask> using SchemeTaskManager = core::webview::CoreSchemeTaskManager<SchemeTask>;
}
