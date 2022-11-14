#include <socket/socket.hh>
#include <assert.h>

import ssc.runtime;
import ssc.json;
import ssc.dns;
import ssc.log;

using namespace ssc;
using ssc::dns::DNS;
using ssc::runtime::Runtime;

int main () {
  Runtime runtime;

  auto lookupCalled = false;
  auto options = DNS::LookupOptions { "sockets.sh" };
  auto& dns = runtime.dns;
  dns.lookup(options, [&](auto seq, JSON::Any json, auto post) {
    auto result = json.as<JSON::Object>();

    if (result.has("data")) {
      auto data = result.get("data").as<JSON::Object>();
      auto family = data["family"].as<JSON::Number>();
      auto address = data["address"].as<JSON::String>();

      assert(family.value() == 4 || family.value() == 6);;
      assert(address.size() > 0);
    } else if (result.has("err")) {
      auto err = result.get("err").as<JSON::Object>();
      auto message = err.get("message");
      log::error(message);
    }

    lookupCalled = true;
    runtime.stop();
  });

  runtime.wait();

  assert(lookupCalled);

  return 0;
}
