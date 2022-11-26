#include <socket/socket.hh>
#include <assert.h>

using namespace ssc;

int main () {
  assert(version::VERSION_FULL_STRING.size() > 0);
  assert(version::VERSION_HASH_STRING.size() > 0);
  assert(version::VERSION_STRING.size() > 0);
  return 0;
}
