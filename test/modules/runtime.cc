#include <assert.h>
#include <new> // @TODO(jwerle): move this to `platform.hh` or similar

import ssc.runtime;
import ssc.init;
import ssc.json;
import ssc.dns;
import ssc.log;

using namespace ssc;
using ssc::dns::DNS;
using ssc::runtime::Runtime;
using ssc::init;

int main () {
  Runtime runtime;

  ssc::init(&runtime);

  auto lookupCalled = false;
  auto options = DNS::LookupOptions { "sockets.sh" };
  auto dns = runtime.interfaces.dns.get()->as<DNS>();
  dns->lookup(options, [&](auto seq, JSON::Any json, auto post) {
    auto data = json.as<JSON::Object>().get("data").as<JSON::Object>();
    auto family = data["family"].as<JSON::Number>();
    auto address = data["address"].as<JSON::String>();

    assert(family.value() == 4 || family.value() == 6);;
    assert(address.size() > 0);
    lookupCalled = true;
    runtime.stop();
  });

  runtime.wait();

  assert(lookupCalled);

  return 0;
}
