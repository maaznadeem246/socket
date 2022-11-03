#include <assert.h>

import ssc.string;
import ssc.env;
import ssc.log;

using namespace ssc;
using String = ssc::string::String;

int main () {
  auto USER = env::get("USER");
  assert(USER.size() > 0);
  env::set("FOO", "BAR");
  auto FOO = env::get("FOO");
  assert(FOO == String("BAR"));
  return 0;
}
