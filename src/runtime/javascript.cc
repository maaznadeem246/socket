#include "runtime.hh"

namespace ssc::runtime {
  Script::Script (const Script& script) {
    this->name = script.name;
    this->source = script.source;
  }

  Script::Script (const String& source) {
    this->source = source;
  }

  Script::Script (const String& name, const String& source) {
    this->name = name;
    this->source = source;
  }

  String Script::str () const {
    auto string = String(";(() => {\n" + trim(this->source) + "\n})();");

    if (this->name.size() > 0) {
      string += "\n//# sourceURL=" + this->name + "\n";
    }

    return string;
  }

  JSON::String Script::json () const {
    return JSON::String(this->str());
  }

  inline String createJavaScriptSource (const String& name, const String& source) {
    return Script(name, source).str();
  }

  inline String getEmitToRenderProcessJavaScript (
    const String& event,
    const String& value,
    const String& target,
    const JSON::Object& options
  ) {
    return createJavaScript("emit-to-render-process.js",
      "const name = decodeURIComponent(`" + event + "`);                   \n"
      "const value = `" + value + "`;                                      \n"
      "const target = " + target + ";                                      \n"
      "const options = " + options.str() + ";                              \n"
      "let detail = value;                                                 \n"
      "                                                                    \n"
      "if (typeof value === 'string') {                                    \n"
      "  try {                                                             \n"
      "    detail = decodeURIComponent(value);                             \n"
      "    detail = JSON.parse(detail);                                    \n"
      "  } catch (err) {                                                   \n"
      "    if (!detail) {                                                  \n"
      "      console.error(`${err.message} (${value})`);                   \n"
      "      return;                                                       \n"
      "    }                                                               \n"
      "  }                                                                 \n"
      "}                                                                   \n"
      "                                                                    \n"
      "const event = new window.CustomEvent(name, { detail, ...options }); \n"
      "target.dispatchEvent(event);                                        \n"
    );
  }

  inline String getEmitToRenderProcessJavaScript (
    const String& event,
    const String& value
  ) {
    return getEmitToRenderProcessJavaScript(event, value, "window", JSON::Object {});
  }

  inline String getResolveMenuSelectionJavaScript (
    const String& seq,
    const String& title,
    const String& parent
  ) {
    return createJavaScript("resolve-menu-selection.js",
      "const detail = {                                           \n"
      "  title: decodeURIComponent(`" + title + "`),              \n"
      "  parent: decodeURIComponent(`" + parent + "`),            \n"
      "  state: '0'                                               \n"
      "};                                                         \n"
      "                                                           \n"
      " if (" + seq + " > 0 && window._ipc['R" + seq + "']) {     \n"
      "   window._ipc['R" + seq + "'].resolve(detail);            \n"
      "   delete window._ipc['R" + seq + "'];                     \n"
      "   return;                                                 \n"
      " }                                                         \n"
      "                                                           \n"
      "const event = new window.CustomEvent('menuItemSelected', { \n"
      "  detail                                                   \n"
      "});                                                        \n"
      "                                                           \n"
      "window.dispatchEvent(event);                               \n"
    );
  }

  inline String getResolveToRenderProcessJavaScript (
    const String& seq,
    const String& state,
    const String& value
  ) {
    return createJavaScript("resolve-to-render-process.js",
      "const seq = String('" + seq + "');                    \n"
      "const value = '" + value + "';                        \n"
      "const index = window.__args.index;                    \n"
      "const state = Number('" + state + "');                \n"
      "const eventName = `resolve-${index}-${seq}`;          \n"
      "let detail = value;                                   \n"
      "                                                      \n"
      "if (typeof value === 'string') {                      \n"
      "  try {                                               \n"
      "    detail = decodeURIComponent(value);               \n"
      "    detail = JSON.parse(detail);                      \n"
      "  } catch (err) {                                     \n"
      "    if (!detail) {                                    \n"
      "      console.error(`${err.message} (${value})`);     \n"
      "      return;                                         \n"
      "    }                                                 \n"
      "  }                                                   \n"
      "}                                                     \n"
      "                                                      \n"
      "if (detail?.err) {                                    \n"
      "  let err = detail?.err ?? detail;                    \n"
      "  if (typeof err === 'string') {                      \n"
      "    err = new Error(err);                             \n"
      "  }                                                   \n"
      "                                                      \n"
      "  detail = { err };                                   \n"
      "} else if (detail?.data) {                            \n"
      "  detail = { ...detail }                              \n"
      "} else {                                              \n"
      "  detail = { data: detail }                           \n"
      "}                                                     \n"
      "                                                      \n"
      "const event = new CustomEvent(eventName, { detail }); \n"
      "window.dispatchEvent(event);                          \n"
    );
  }

}
#endif
