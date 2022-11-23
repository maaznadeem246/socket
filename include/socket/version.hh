#ifndef SSC_SOCKET_VERSION_HH
#define SSC_SOCKET_VERSION_HH

#include "common.hh"

namespace ssc::version {
  inline const auto VERSION_FULL_STRING = ToString(STR_VALUE(SSC_VERSION) " (" STR_VALUE(SSC_VERSION_HASH) ")");
  inline const auto VERSION_HASH_STRING = ToString(STR_VALUE(SSC_VERSION_HASH));
  inline const auto VERSION_STRING = ToString(STR_VALUE(SSC_VERSION));
}
#endif
