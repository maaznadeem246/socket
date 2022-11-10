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
    log::info(request.url);
    return true;
  });

  SchemeRequest request = {
    .method = "GET",
    .url = "ipc://hello?value=world"
  };

  log::info(request.url);

  schemeHandler.onSchemeRequest(request);
  assert(called);
}

int main () {
  test_ipc_scheme_handler();
  return 0;
}
