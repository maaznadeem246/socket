#include "javascript.hh"
#include "window.hh"
#include "internal/window.hh"
#include "string.hh"

using namespace ssc::core::string;

namespace ssc::core::window {
  CoreWindow::CoreWindow (
    CoreApplication& app,
    const CoreWindowOptions opts,
    webview::CoreIPCSchemeRequestRouteCallback onIPCSchemeRequestRouteCallback
  ) : app(app), opts(opts)  {
    // preload script for normalizing the interface to be cross-platform.
    auto preloadScript = javascript::Script("preload.js", ToString(createPreload(opts)));
    this->internals = new CoreWindowInternals(this, opts);
    this->webview = new CoreWebView(
      this,
      opts.dataManager,
      onIPCSchemeRequestRouteCallback,
      preloadScript
    );

    this->initialize();
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

  void CoreWindow::exit (int code) {
    if (onExit != nullptr) onExit(code);
  }

  void CoreWindow::eval (Script script) {
    this->eval(script.str());
  }
}
