#include <socket/config.hh>
#include "core/config.hh"
#include "core/utils.hh"

namespace ssc::config {
  bool isDebugEnabled () {
  #if defined(DEBUG) && DEBUG
    return true;
  #endif
    return false;
  }

  void init (GlobalConfig& config) {
    config.set(core::utils::decodeURIComponent(STR_VALUE(SSC_CONFIG)));
  }

  int getServerPort () {
  #if defined(PORT) && PORT >= 0
    return (int) PORT;
  #endif

    return 0;
  }
}
