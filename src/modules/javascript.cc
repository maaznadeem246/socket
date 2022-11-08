module; // global
#include "../core/platform.hh"

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
import ssc.json;
import ssc.string;

using String = ssc::string::String;
using ssc::string::trim;

export namespace ssc::javascript {
  #define SSC_INLINE_INCLUDE
  #include "../core/javascript.hh"
}
