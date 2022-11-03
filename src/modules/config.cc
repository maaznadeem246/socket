module;
#include <map>
#include <string>

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
import ssc.string;
import ssc.types;
import ssc.json;

using namespace ssc::types;
using namespace ssc::string;

export namespace ssc::config {
  class Config {
    String source;
    Map data;
    public:
      Config () = default;
      Config (const String& source) {
        this->source = source;

        auto entries = split(source, '\n');

        for (auto entry : entries) {
          auto index = entry.find_first_of(':');

          if (index >= 0 && index <= entry.size()) {
            auto key = entry.substr(0, index);
            auto value = entry.substr(index + 1);

            this->data[trim(key)] = trim(value);
          }
        }
      }

      const auto operator [] (const String& key) const {
        return this->data.at(key);
      }

      auto &operator [] (const String& key) {
        return this->data[key];
      }

      const auto get (const String& key) const {
        return this->data.at(key);
      }

      auto set (const String& key, const String& value) {
        this->data[key] = value;
      }

      const auto str () const {
        return this->source;
      }

      auto size () const {
        return this->data.size();
      }

      auto json () const {
        return JSON::Object(this->data);
      }
  };
}
