#include <socket/socket.hh>
#include <assert.h>

using namespace ssc;

int main () {
  auto USER = env::get("USER");
  assert(USER.size() > 0);
  env::set("FOO", "BAR");
  auto FOO = env::get("FOO");
  assert(FOO == String("BAR"));
  return 0;
}
