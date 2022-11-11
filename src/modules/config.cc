module;
#include "../core/config.hh"

/**
 * @module ssc.string
 * @description Various utiltity string functions
 * @example
 * import ssc.config
 * using Config = ssc::config::Config;
 * namespace ssc {
 *   Config config(configSource);
 *   auto value = config.get("key");
 *   auto other = config["other"];
 * }
 */
export module ssc.config;

export namespace ssc::config {
  using Config = ssc::core::config::Config;
}
