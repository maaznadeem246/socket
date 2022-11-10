#include <assert.h>
#include "../../src/core/platform.hh"

import ssc.webview;
import ssc.types;
import ssc.ipc;
import ssc.log;

using namespace ssc;
using namespace ssc::webview;
using namespace ssc::types;
using namespace ssc::ipc;
using namespace ssc::ipc::data;

void test_ipc_scheme_handler () {
  DataManager dataManager;
  bool called = false;
  IPCSchemeHandler schemeHandler(&dataManager, [&](const auto& request) {
    called = true;
    assert(request.method == "GET");
    assert(request.url == "ipc://hello?value=world");
    assert(request.message.name == "hello");
    assert(request.message.value == "world");
    return true;
  });

  SchemeRequest request = { String("ipc://hello?value=world") };

  schemeHandler.onSchemeRequest(request);
  assert(called);
}

int main () {
  test_ipc_scheme_handler();
  return 0;
}
