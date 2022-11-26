#include <socket/socket.hh>
#include <assert.h>

using namespace ssc;

int main () {
  String string = format("  $S  ", String("hello world"));
  String trimmed = trim(string);
  String replaced = replace(trimmed, "hello", "good byte");
  Vector<String> parts = split(replaced, 0);
  auto expected = String("good byte world");
  for (int i = 0; i < expected.size(); ++i) {
    assert(expected[i] == parts[i][0]);
  }
  return 0;
}
