#include <socket/socket.hh>
#include <assert.h>

using namespace ssc;
using namespace ssc::runtime;

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
