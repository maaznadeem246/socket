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
  using core::utils::addrToIPv4; // NOLINT(misc-unused-using-decls)
  using core::utils::addrToIPv6; // NOLINT(misc-unused-using-decls)
  using core::utils::decodeUTF8; // NOLINT(misc-unused-using-decls)
  using core::utils::decodeURIComponent; // NOLINT(misc-unused-using-decls)
  using core::utils::encodeURIComponent; // NOLINT(misc-unused-using-decls)
}
