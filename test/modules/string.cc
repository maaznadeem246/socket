#include <socket/runtime.hh>
#include <assert.h>

import ssc.string;
import ssc.types;

int main () {
  using namespace ssc::types;
  using namespace ssc::string;

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
