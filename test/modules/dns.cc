import ssc.runtime;
import ssc.string;
import ssc.json;
import ssc.dns;
import ssc.log;

#include <new>
#include <assert.h>

using namespace ssc;
using ssc::dns::DNS;
using ssc::string::String;
using ssc::runtime::Runtime;

auto test_family4_lookup_passed = false;
auto test_family6_lookup_passed = false;
int pending = 0;

void test_family4_lookup (Runtime& runtime, const DNS::LookupOptions& options) {
  pending++;

  auto& dns = runtime.dns;
  dns.lookup(options, [&](auto seq, JSON::Any json, auto post) {
    auto data = json.as<JSON::Object>().get("data").as<JSON::Object>();
    auto family = data["family"].as<JSON::Number>();
    auto address = data["address"].as<JSON::String>();

    assert(family.value() == 4);
    assert(address.size() > 0);

    test_family4_lookup_passed = true;

    if (--pending == 0) {
      runtime.stop();
    }
  });
}

void test_family6_lookup (Runtime& runtime, const DNS::LookupOptions& options) {
  pending++;

  auto& dns = runtime.dns;
  dns.lookup(options, [&](auto seq, JSON::Any json, auto post) {
    auto data = json.as<JSON::Object>().get("data").as<JSON::Object>();
    auto family = data["family"].as<JSON::Number>();
    auto address = data["address"].as<JSON::String>();

    assert(family.value() == 6);
    assert(address.size() > 0);

    test_family6_lookup_passed = true;

    if (--pending == 0) {
      runtime.stop();
    }
  });
}

int main () {
  Runtime runtime;

  test_family4_lookup(runtime, DNS::LookupOptions { "sockets.sh", 4 });
  test_family6_lookup(runtime, DNS::LookupOptions { "cloudflare.com", 6 });

  runtime.wait();

  assert(test_family4_lookup_passed);
  assert(test_family6_lookup_passed);

  return 0;
}