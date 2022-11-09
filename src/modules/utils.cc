module;
#include "../core/platform.hh"

/**
 * @module ssc.utils
 * @description Various utility helper functions
 * @example
 * import ssc.util;
 * namespace ssc {
 *   auto u64 = utils::rand64();
 * }
 */
export module ssc.utils;

export namespace ssc::utils {
  #define SSC_INLINE_INCLUDE
  #include "../core/utils.hh"
}
