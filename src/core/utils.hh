#ifndef SSC_CORE_UTILS_HH
#define SSC_CORE_UTILS_HH

#include <socket/platform.hh>
#include <cmath>
#include "types.hh"
#include "string.hh"

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

  inline String addrToIPv4 (struct sockaddr_in *sin) {
    char buf[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &sin->sin_addr, buf, INET_ADDRSTRLEN);
    return String(buf);
  }

  inline String addrToIPv6 (struct sockaddr_in6 *sin) {
    char buf[INET6_ADDRSTRLEN];
    inet_ntop(AF_INET6, &sin->sin6_addr, buf, INET6_ADDRSTRLEN);
    return String(buf);
  }

  #define IN_URANGE(c, a, b) (                \
    (unsigned char) c >= (unsigned char) a && \
    (unsigned char) c <= (unsigned char) b    \
  )

  //
  // All ipc uses a URI schema, so all ipc data needs to be
  // encoded as a URI component. This prevents escaping the
  // protocol.
  //
  static const signed char HEX2DEC[256] = {
    /*       0  1  2  3   4  5  6  7   8  9  A  B   C  D  E  F */
    /* 0 */ -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1,
    /* 1 */ -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1,
    /* 2 */ -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1,
    /* 3 */  0, 1, 2, 3,  4, 5, 6, 7,  8, 9,-1,-1, -1,-1,-1,-1,

    /* 4 */ -1,10,11,12, 13,14,15,-1, -1,-1,-1,-1, -1,-1,-1,-1,
    /* 5 */ -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1,
    /* 6 */ -1,10,11,12, 13,14,15,-1, -1,-1,-1,-1, -1,-1,-1,-1,
    /* 7 */ -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1,

    /* 8 */ -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1,
    /* 9 */ -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1,
    /* A */ -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1,
    /* B */ -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1,

    /* C */ -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1,
    /* D */ -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1,
    /* E */ -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1,
    /* F */ -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1
  };

  static const char SAFE[256] = {
    /*      0 1 2 3  4 5 6 7  8 9 A B  C D E F */
    /* 0 */ 0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0,
    /* 1 */ 0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0,
    /* 2 */ 0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0,
    /* 3 */ 1,1,1,1, 1,1,1,1, 1,1,0,0, 0,0,0,0,

    /* 4 */ 0,1,1,1, 1,1,1,1, 1,1,1,1, 1,1,1,1,
    /* 5 */ 1,1,1,1, 1,1,1,1, 1,1,1,0, 0,0,0,0,
    /* 6 */ 0,1,1,1, 1,1,1,1, 1,1,1,1, 1,1,1,1,
    /* 7 */ 1,1,1,1, 1,1,1,1, 1,1,1,0, 0,0,0,0,

    /* 8 */ 0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0,
    /* 9 */ 0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0,
    /* A */ 0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0,
    /* B */ 0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0,

    /* C */ 0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0,
    /* D */ 0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0,
    /* E */ 0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0,
    /* F */ 0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0
  };

  inline size_t decodeUTF8 (char *output, const char *input, size_t length) {
    unsigned char cp = 0; // code point
    unsigned char lower = 0x80;
    unsigned char upper = 0xBF;

    int x = 0; // cp needed
    int y = 0; // cp  seen
    int size = 0; // output size

    for (int i = 0; i < length; ++i) {
      auto b = (unsigned char) input[i];

      if (b == 0) {
        output[size++] = 0;
        continue;
      }

      if (x == 0) {
        // 1 byte
        if (IN_URANGE(b, 0x00, 0x7F)) {
          output[size++] = b;
          continue;
        }

        if (!IN_URANGE(b, 0xC2, 0xF4)) {
          break;
        }

        // 2 byte
        if (IN_URANGE(b, 0xC2, 0xDF)) {
          x = 1;
          cp = b - 0xC0;
        }

        // 3 byte
        if (IN_URANGE(b, 0xE0, 0xEF)) {
          if (b == 0xE0) {
            lower = 0xA0;
          } else if (b == 0xED) {
            upper = 0x9F;
          }

          x = 2;
          cp = b - 0xE0;
        }

        // 4 byte
        if (IN_URANGE(b, 0xF0, 0xF4)) {
          if (b == 0xF0) {
            lower = 0x90;
          } else if (b == 0xF4) {
            upper = 0x8F;
          }

          x = 3;
          cp = b - 0xF0;
        }

        cp = cp * pow(64, x);
        continue;
      }

      if (!IN_URANGE(b, lower, upper)) {
        lower = 0x80;
        upper = 0xBF;

        // revert
        cp = 0;
        x = 0;
        y = 0;
        i--;
        continue;
      }

      lower = 0x80;
      upper = 0xBF;
      y++;
      cp += (b - 0x80) * pow(64, x - y);

      if (y != x) {
        continue;
      }

      output[size++] = cp;
      // continue to next
      cp = 0;
      x = 0;
      y = 0;
    }

    return size;
  }

  inline String decodeURIComponent(const String& sSrc) {

    // Note from RFC1630:  "Sequences which start with a percent sign
    // but are not followed by two hexadecimal characters (0-9, A-F) are reserved
    // for future extension"

    auto s = string::replace(sSrc, "\\+", " ");
    const unsigned char* pSrc = (const unsigned char *) s.c_str();
    const int SRC_LEN = (int) sSrc.length();
    const unsigned char* const SRC_END = pSrc + SRC_LEN;
    const unsigned char* const SRC_LAST_DEC = SRC_END - 2;

    char* const pStart = new char[SRC_LEN];
    char* pEnd = pStart;

    while (pSrc < SRC_LAST_DEC) {
      if (*pSrc == '%') {
        char dec1, dec2;
        if (-1 != (dec1 = HEX2DEC[*(pSrc + 1)])
            && -1 != (dec2 = HEX2DEC[*(pSrc + 2)])) {

            *pEnd++ = (dec1 << 4) + dec2;
            pSrc += 3;
            continue;
        }
      }
      *pEnd++ = *pSrc++;
    }

    // the last 2- chars
    while (pSrc < SRC_END) {
      *pEnd++ = *pSrc++;
    }

    String sResult(pStart, pEnd);
    delete [] pStart;
    return sResult;
  }

  inline String encodeURIComponent (const String& sSrc) {
    const char DEC2HEX[16 + 1] = "0123456789ABCDEF";
    const unsigned char* pSrc = (const unsigned char*) sSrc.c_str();
    const int SRC_LEN = (int) sSrc.length();
    unsigned char* const pStart = new unsigned char[SRC_LEN* 3];
    unsigned char* pEnd = pStart;
    const unsigned char* const SRC_END = pSrc + SRC_LEN;

    for (; pSrc < SRC_END; ++pSrc) {
      if (SAFE[*pSrc]) {
        *pEnd++ = *pSrc;
      } else {
        // escape this char
        *pEnd++ = '%';
        *pEnd++ = DEC2HEX[*pSrc >> 4];
        *pEnd++ = DEC2HEX[*pSrc & 0x0F];
      }
    }

    String sResult((char*) pStart, (char*) pEnd);
    delete [] pStart;
    return sResult;
  }
}
#endif
