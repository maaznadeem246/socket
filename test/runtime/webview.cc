#include <socket/socket.hh>
#include <assert.h>

using namespace ssc;
using namespace ssc::runtime;
using namespace ssc::runtime::webview;

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

  SchemeRequest request = { String("ipc://hello?value=world") };

  schemeHandler.onSchemeRequest(request);
  assert(called);
}

int main () {
  test_ipc_scheme_handler();
  return 0;
}
