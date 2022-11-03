#include <assert.h>

import ssc.javascript;
import ssc.string;
import ssc.log;

using namespace ssc;
using ssc::string::String;
using ssc::javascript::Script;

int main () {
  Script script("hello.js", "1 + 2 + 3");
  assert(
    script.str() ==
    String(
      ";(() => {\n"
      "1 + 2 + 3\n"
      "})();\n"
      "//# sourceURL=hello.js\n"
    )
  );
  return 0;
}
