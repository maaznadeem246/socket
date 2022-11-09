module;
#include "../core/platform.hh"

/**
 * @module ssc.version;
 * @description Library and command line version value constants.
 * @example
 * import ssc.version;
 * import ssc.log;
 *
 * namespace ssc {
 *   log::info(version::VERSION_FULL_STRING);
 *   log::info(version::VERSION_HASH_STRING);
 *   log::info(version::VERSION_STRING);
 * }
 */
export module ssc.version;
import ssc.string;

using namespace ssc::string;

export namespace ssc::version {
  #define SSC_INLINE_INCLUDE
  #include "../core/version.hh"
}
