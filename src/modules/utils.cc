module;
#include "../core/utils.hh"

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
  const auto& rand64 = ssc::core::utils::rand64;
}
