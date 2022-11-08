module;
#include "../core/platform.hh"

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
import ssc.string;

using String = ssc::string::String;

export namespace ssc::env {
  #define SSC_INLINE_INCLUDE
  #include "../core/env.hh"
}
