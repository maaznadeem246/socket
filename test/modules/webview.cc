#include <socket/socket.hh>
#include <assert.h>

import ssc.webview;
import ssc.string;
import ssc.types;
import ssc.json;
import ssc.data;
import ssc.ipc;
import ssc.log;

#include <new>

using namespace ssc;
using namespace ssc::data;
using namespace ssc::types;
using namespace ssc::webview;

void test_ipc_scheme_handler () {
  DataManager dataManager;
  bool called = false;
  IPCSchemeHandler schemeHandler(&dataManager, [&](const auto& request) {
    called = true;
    assert(request.method == String("GET"));
    assert(request.url == String("ipc://hello?value=world"));
    assert(request.message.name == String("hello"));
    assert(request.message.value == String("world"));
    return true;
  });

  IPCSchemeRequest request = { String("ipc://hello?value=world") };

  schemeHandler.onSchemeRequest(request);
  assert(called);
}

int main () {
  test_ipc_scheme_handler();
  return 0;
}
