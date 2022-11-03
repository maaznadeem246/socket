#include <assert.h>
#include <new>

import ssc.loop;
import ssc.log;

using namespace ssc;
using Loop = ssc::loop::Loop;

int main () {
  Loop loop;
  bool called = false;
  loop.dispatch([&]() {
  assert(called == false);
    called = true;
    loop.stop();
  });
  loop.wait();
  assert(called == true);
  return 0;
}
