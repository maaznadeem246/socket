#include "javascript.hh"
#include "window.hh"

namespace ssc::core::window {
  inline String getResolveToMainProcessMessage (
    const String& seq,
    const String& state,
    const String& value
  ) {
    return String("ipc://resolve?seq=" + seq + "&state=" + state + "&value=" + value);
  }

  CoreWindow::CoreWindow (CoreApplication& app) : app(app) {
    this->webview = new CoreWebView();
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
