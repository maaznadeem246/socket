#include <socket/socket.hh>
#include <assert.h>

import ssc.config;
import ssc.string;
import ssc.log;

using namespace ssc::string;
using namespace ssc::config;

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

  assert(
    config.json().str() ==
    String(R"JSON({"key":"new value","new key":"value","overwritten":"reachable"})JSON")
  );
  return 0;
}
