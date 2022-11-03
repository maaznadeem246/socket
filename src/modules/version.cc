module;

#include "../common.hh"

#ifndef SSC_VERSION
#define SSC_VERSION ""
#endif

#ifndef SSC_VERSION_HASH
#define SSC_VERSION_HASH ""
#endif

export module ssc.version;
import ssc.string;

using namespace ssc::string;

export namespace ssc::version {
  inline const auto VERSION_FULL_STRING = ToString(STR_VALUE(SSC_VERSION) " (" STR_VALUE(SSC_VERSION_HASH) ")");
  inline const auto VERSION_HASH_STRING = ToString(STR_VALUE(SSC_VERSION_HASH));
  inline const auto VERSION_STRING = ToString(STR_VALUE(SSC_VERSION));
}
