#include <socket/settings.hh>

namespace ssc::settings {
  const char *source;
  void init () {
    source = STR_VALUE(SSC_SETTINGS);
  }

  const char* getSourceString () {
    return source;
  }
}
