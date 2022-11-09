#include <assert.h>

import ssc.headers;
import ssc.string;
import ssc.log;

using namespace ssc::headers;
using namespace ssc::string;
using namespace ssc;

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
