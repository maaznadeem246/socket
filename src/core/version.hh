#ifndef SSC_CORE_VERSION_HH
#define SSC_CORE_VERSION_HH

#include <socket/platform.hh>
#include "string.hh"

namespace ssc::core::version {
  using namespace ssc::core::string;
  inline const auto VERSION_FULL_STRING = ToString(STR_VALUE(SSC_VERSION) " (" STR_VALUE(SSC_VERSION_HASH) ")");
  inline const auto VERSION_HASH_STRING = ToString(STR_VALUE(SSC_VERSION_HASH));
  inline const auto VERSION_STRING = ToString(STR_VALUE(SSC_VERSION));
}
#endif
