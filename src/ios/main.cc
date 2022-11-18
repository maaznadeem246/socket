#include <socket/socket.hh>
#include "start.hh"

import ssc.application;
import ssc.config;
import ssc.window;

using namespace ssc::application;
using namespace ssc::config;
using namespace ssc::window;

int main (const int argc, const char **argv) {
  Application app(0, argc, argv);

  return ssc::start(argc, argv);
}
