module;
#include "../core/platform.hh"

/**
 * @module ssc.version;
 * @description TODO
 * @example
 * TODO
 */
export module ssc.version;
import ssc.string;

using namespace ssc::string;

export namespace ssc::version {
  #define SSC_INLINE_INCLUDE
  #include "../core/version.hh"
}
