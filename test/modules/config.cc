#include <assert.h>

import ssc.config;
import ssc.string;

using String = ssc::string::String;
using Config = ssc::config::Config;

auto constexpr source = R"CONFIG(
# a comment
key: value

# another comment
overwritten: unreachable
overwritten: reachable
)CONFIG";

int main () {
  auto config = Config(source);
  assert(config["key"] == String("value"));
  assert(config.get("overwritten") == String("reachable"));
  assert(config.size() == 2);
  config.set("key", "new value");
  assert(config["key"] == String("new value"));
  config["new key"] = "value";
  assert(config.size() == 3);
  return 0;
}
