#ifndef SSC_CORE_UTILS_HH
#if !defined(SSC_INLINE_INCLUDE)
#define SSC_CORE_UTILS_HH
#include "types.hh"
#endif

#define IMAX_BITS(m) ((m)/((m) % 255+1) / 255 % 255 * 8 + 7-86 / ((m) % 255+12))
#define RAND_MAX_WIDTH IMAX_BITS(RAND_MAX)

#if !defined(SSC_INLINE_INCLUDE)
namespace ssc::utils {
  using namespace ssc::types;
#endif
  inline uint64_t rand64 (void) {
    uint64_t r = 0;
    for (int i = 0; i < 64; i += RAND_MAX_WIDTH) {
      r <<= RAND_MAX_WIDTH;
      r ^= (unsigned) rand();
    }
    return r;
  }

#if !defined(SSC_INLINE_INCLUDE)
}
#endif
#endif
