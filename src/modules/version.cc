module;
#include "../core/version.hh"

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

export namespace ssc::version {
  using ssc::core::version::VERSION_FULL_STRING; // NOLINT(misc-unused-using-decls)
  using ssc::core::version::VERSION_HASH_STRING; // NOLINT(misc-unused-using-decls)
  using ssc::core::version::VERSION_STRING; // NOLINT(misc-unused-using-decls)
}
