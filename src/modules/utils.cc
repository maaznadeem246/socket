module;
#include <stddef.h>
#include <stdlib.h>

/**
 * @module ssc.utils
 * @description Various utility helper functions
 * @example
 * TODO
 */
export module ssc.utils;
import ssc.string;

using String = ssc::string::String;

#define IMAX_BITS(m) ((m)/((m) % 255+1) / 255 % 255 * 8 + 7-86 / ((m) % 255+12))
#define RAND_MAX_WIDTH IMAX_BITS(RAND_MAX)

export namespace ssc::utils {
  inline uint64_t rand64 (void) {
    uint64_t r = 0;
    for (int i = 0; i < 64; i += RAND_MAX_WIDTH) {
      r <<= RAND_MAX_WIDTH;
      r ^= (unsigned) rand();
    }
    return r;
  }
}
