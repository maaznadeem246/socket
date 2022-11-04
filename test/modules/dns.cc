#include <assert.h>
#include <new> // @TODO(jwerle): move this to `platform.hh` or similar

import ssc.runtime;
import ssc.string;
import ssc.init;
import ssc.json;
import ssc.dns;
import ssc.log;

using namespace ssc;
using ssc::init;
using ssc::dns::DNS;
using ssc::string::String;
using ssc::runtime::Runtime;

auto test_ipv4_passed = false;
auto test_ipv6_passed = false;
int pending = 0;

void test_ipv4 (Runtime& runtime, const DNS::LookupOptions& options) {
  pending++;

  auto dns = runtime.interfaces.dns.get()->as<DNS>();
  dns->lookup(options, [&](auto seq, JSON::Any json, auto post) {
    auto data = json.as<JSON::Object>().get("data").as<JSON::Object>();
    auto family = data["family"].as<JSON::Number>();
    auto address = data["address"].as<JSON::String>();

    assert(family.value() == 4);
    assert(address.size() > 0);

    test_ipv4_passed = true;

    if (--pending == 0) {
      runtime.stop();
    }
  });
}

void test_ipv6 (Runtime& runtime, const DNS::LookupOptions& options) {
  pending++;

  auto dns = runtime.interfaces.dns.get()->as<DNS>();
  dns->lookup(options, [&](auto seq, JSON::Any json, auto post) {
    auto data = json.as<JSON::Object>().get("data").as<JSON::Object>();
    auto family = data["family"].as<JSON::Number>();
    auto address = data["address"].as<JSON::String>();

    assert(family.value() == 6);
    assert(address.size() > 0);

    test_ipv6_passed = true;

    if (--pending == 0) {
      runtime.stop();
    }
  });
}

int main () {
  Runtime runtime;

  ssc::init(&runtime);

  test_ipv4(runtime, DNS::LookupOptions { "sockets.sh", 4 });
  test_ipv6(runtime, DNS::LookupOptions { "cloudflare.com", 6 });

  runtime.wait();

  assert(test_ipv4_passed);
  assert(test_ipv6_passed);

  return 0;
}
