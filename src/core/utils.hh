#ifndef SSC_CORE_UTILS_HH
#define SSC_CORE_UTILS_HH

#include <socket/platform.hh>
#include <cmath>
#include "types.hh"

#define IMAX_BITS(m) ((m)/((m) % 255+1) / 255 % 255 * 8 + 7-86 / ((m) % 255+12))
#define RAND_MAX_WIDTH IMAX_BITS(RAND_MAX)

namespace ssc::core::utils {
  using namespace ssc::core::types;
  inline uint64_t rand64 (void) {
    uint64_t r = 0;
    for (int i = 0; i < 64; i += RAND_MAX_WIDTH) {
      r <<= RAND_MAX_WIDTH;
      r ^= (unsigned) rand();
    }
    return r;
  }
}
#endif
