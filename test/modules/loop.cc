#include <assert.h>
#include <new>

import ssc.loop;
import ssc.log;

using namespace ssc;
using Loop = ssc::loop::Loop;

int main () {
  Loop loop;
  bool called = false;
  assert(!loop.isAlive());
  assert(!loop.isRunning());
  assert(!loop.isInitialized());
  loop.dispatch([&]() {
    assert(loop.isAlive());
    assert(loop.isRunning());
    assert(loop.isInitialized());
    assert(called == false);
    called = true;
    loop.stop();
    assert(!loop.isRunning());
  });
  assert(loop.isAlive());
  assert(loop.isRunning());
  assert(loop.isInitialized());
  loop.wait();
  assert(loop.isAlive());
  assert(!loop.isRunning());
  assert(loop.isInitialized());
  assert(called == true);
  return 0;
}
