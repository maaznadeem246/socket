#ifndef SSC_CORE_VERSION_HH
#define SSC_CORE_VERSION_HH

#if !defined(SSC_INLINE_INCLUDE)
#include "string.hh"
#include "common.hh"
#endif

#if !defined(SSC_INLINE_INCLUDE)
namespace ssc::version {
  using namespace ssc::string;
#endif
  inline const auto VERSION_FULL_STRING = ToString(STR_VALUE(SSC_VERSION) " (" STR_VALUE(SSC_VERSION_HASH) ")");
  inline const auto VERSION_HASH_STRING = ToString(STR_VALUE(SSC_VERSION_HASH));
  inline const auto VERSION_STRING = ToString(STR_VALUE(SSC_VERSION));

#if !defined(SSC_INLINE_INCLUDE)
}
#endif
#endif
