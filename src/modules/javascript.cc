module; // global
#include "../core/javascript.hh"

/**
 * @module ssc.javascript
 * @description TODO
 * @example
 * import ssc.javascript;
 * using namespace ssc::javascript;
 * namespace ssc {
 * }
 */
export module ssc.javascript;

export namespace ssc::javascript {
  using ssc::core::javascript::createJavaScript; // NOLINT(misc-unused-using-decls)
  using ssc::core::javascript::getEmitToRenderProcessJavaScript; // NOLINT(misc-unused-using-decls)
  using ssc::core::javascript::getResolveMenuSelectionJavaScript;  // NOLINT(misc-unused-using-decls)
  using ssc::core::javascript::getResolveToRenderProcessJavaScript; // NOLINT(misc-unused-using-decls)
  using ssc::core::javascript::Script; // NOLINT(misc-unused-using-decls)
}
