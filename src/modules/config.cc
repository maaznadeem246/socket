module;
#include <socket/config.hh>
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
  using ssc::config::isDebugEnabled; // NOLINT
  using ssc::config::getServerPort; // NOLINT
  using Config = ssc::core::config::Config;
}
