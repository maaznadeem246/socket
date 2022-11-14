#include <socket/socket.hh>
#include <assert.h>

import ssc.application;

using namespace ssc::application;

int main (const int argc, const char** argv) {
  Application app(0, argc, argv);
  app.run();
  return 0;
}
