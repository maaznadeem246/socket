module;
#include "../core/env.hh"

/**
 * @module ssc.env
 * @description Set/get environment variables
 * @example
 * import ssc.env;
 * namespace ssc {
 *   auto USER = env::get("USER");
 *   auto HOME = env::get("HOME");
 *
 *   env::set("KEY=value");
 *   env::set("KEY", "value");
 * }
 * TODO
 */
export module ssc.env;

export namespace ssc::env {
  using ssc::core::env::get; // NOLINT(misc-unused-using-decls)
  using ssc::core::env::set; // NOLINT(misc-unused-using-decls)
}
