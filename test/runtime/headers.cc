#include <socket/socket.hh>
#include <assert.h>

using namespace ssc;
using namespace ssc::runtime;

int main () {
  auto entries = Headers::Entries {
    {"key", "value"},
    {"x-key", "foo bar"},
    {"x-size", 12345}
  };

  assert(Headers().str() == String(""));
  assert(
    Headers(entries).str() ==
    String("key: value\nx-key: foo bar\nx-size: 12345")
  );

  return 0;
}
