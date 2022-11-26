#include <socket/socket.hh>
#include <socket/utils.hh>

namespace ssc::config {
  bool isDebugEnabled () {
  #if defined(DEBUG) && DEBUG
    return true;
  #endif
    return false;
  }

  void init (Config& config) {
    config.set(utils::decodeURIComponent(STR_VALUE(SSC_CONFIG)));
  }

  int getServerPort () {
  #if defined(PORT) && PORT >= 0
    return (int) PORT;
  #endif

    return 0;
  }
}
