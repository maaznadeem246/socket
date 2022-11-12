#include "javascript.hh"
#include "window.hh"
#include "internal/window.hh"
#include "string.hh"

using namespace ssc::core::string;

namespace ssc::core::window {
  inline String getResolveToMainProcessMessage (
    const String& seq,
    const String& state,
    const String& value
  ) {
    return String("ipc://resolve?seq=" + seq + "&state=" + state + "&value=" + value);
  }

  CoreWindow::CoreWindow (
    CoreApplication& app,
    const CoreWindowOptions opts,
    webview::CoreIPCSchemeRequestRouteCallback onIPCSchemeRequestRouteCallback
  ) : app(app), opts(opts)  {
    this->internals = new CoreWindowInternals(opts);
    this->webview = new CoreWebView(
      this,
      opts.dataManager,
      onIPCSchemeRequestRouteCallback
    );

    // Add preload script, normalizing the interface to be cross-platform.
    auto preloadScript = javascript::Script("preload.js", ToString(createPreload(opts)));
    this->webview->addPreloadScript(preloadScript);
  }

  void CoreWindow::resolvePromise (
    const String& seq,
    const String& state,
    const String& value
  ) {
    if (seq.find("R") == 0) {
      this->eval(javascript::getResolveToRenderProcessJavaScript(seq, state, value));
    }

    this->onMessage(getResolveToMainProcessMessage(seq, state, value));
  }
}
