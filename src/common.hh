#ifndef SSC_COMMON_H
#define SSC_COMMON_H

#include <errno.h>
#include <math.h>
#include <stdlib.h>
#include <stdio.h>

#if !defined(_WIN32)
#include <arpa/inet.h>
#include <ifaddrs.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#endif

// macOS/iOS
#if defined(__APPLE__)
#include <TargetConditionals.h>

#if TARGET_OS_IPHONE || TARGET_IPHONE_SIMULATOR
#include <_types/_uint64_t.h>
#include <netinet/in.h>
#include <sys/un.h>
#else
#include <objc/objc-runtime.h>
#endif

#ifndef debug
#define debug(format, ...) NSLog(@format, ##__VA_ARGS__)
#endif
#endif

// Linux
#if defined(__linux__) && !defined(__ANDROID__)
#ifndef debug
#define debug(format, ...) fprintf(stderr, format "\n", ##__VA_ARGS__)
#endif
#endif

// Android (Linux)
#if defined(__linux__) && defined(__ANDROID__)

#ifndef debug
#define debug(format, ...) \
  __android_log_print(     \
      ANDROID_LOG_DEBUG,   \
      __FUNCTION__,        \
      format,              \
      ##__VA_ARGS__        \
    );
#endif
#endif

// Windows
#if defined(_WIN32)
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include <windows.h>

#include <dwmapi.h>
#include <io.h>
#include <tchar.h>
#include <wingdi.h>

#include <future>
#include <signal.h>
#include <shlobj_core.h>
#include <shobjidl.h>

#define isatty _isatty
#define fileno _fileno

#ifndef debug
#define debug(format, ...) fprintf(stderr, format "\n", ##__VA_ARGS__)
#endif
#endif

#include <array>
#include <chrono>
#include <cstdint>
#include <iostream>
#include <filesystem>
#include <fstream>
#include <functional>
#include <span>
#include <thread>

#ifndef DEBUG
#define DEBUG 0
#endif

#ifndef SSC_SETTINGS
#define SSC_SETTINGS ""
#endif

#ifndef SSC_VERSION
#define SSC_VERSION ""
#endif

#ifndef SSC_VERSION_HASH
#define SSC_VERSION_HASH ""
#endif

      /*
#if !DEBUG
#ifdef debug
#undef debug
#endif
#define debug(format, ...)
#endif
*/
#define TO_STR(arg) #arg
#define STR_VALUE(arg) TO_STR(arg)

#define IN_URANGE(c, a, b) (                  \
    (unsigned char) c >= (unsigned char) a && \
    (unsigned char) c <= (unsigned char) b    \
)

#define IMAX_BITS(m) ((m)/((m) % 255+1) / 255 % 255 * 8 + 7-86 / ((m) % 255+12))
#define RAND_MAX_WIDTH IMAX_BITS(RAND_MAX)

namespace ssc {

  using ExitCallback = std::function<void(int code)>;
  using MessageCallback = std::function<void(const String)>;

  inline const auto VERSION_FULL_STRING = ToString(STR_VALUE(SSC_VERSION) " (" STR_VALUE(SSC_VERSION_HASH) ")");
  inline const auto VERSION_HASH_STRING = ToString(STR_VALUE(SSC_VERSION_HASH));
  inline const auto VERSION_STRING = ToString(STR_VALUE(SSC_VERSION));

  inline String encodeURIComponent (const String& sSrc);
  inline String decodeURIComponent (const String& sSrc);
  inline String trim (String str);


  //
  // Reporting on the platform.
  //
  struct PlatformInfo {
    #if defined(__x86_64__) || defined(_M_X64)
      const String arch = "x86_64";
    #elif defined(__aarch64__) || defined(_M_ARM64)
      const String arch = "arm64";
    #else
      const String arch = "unknown";
    #endif

    #if defined(_WIN32)
      const String os = "win32";
      bool mac = false;
      bool ios = false;
      bool win = true;
      bool linux = false;
      bool unix = false;

    #elif defined(__APPLE__)
      bool win = false;
      bool linux = false;

      #if TARGET_OS_IPHONE || TARGET_IPHONE_SIMULATOR
        String os = "ios";
        bool ios = true;
        bool mac = false;
      #else
        String os = "mac";
        bool ios = false;
        bool mac = true;
      #endif

      #if defined(__unix__) || defined(unix) || defined(__unix)
        bool unix = true;
      #else
        bool unix = false;
      #endif

    #elif defined(__linux__)
      #undef linux
      #ifdef __ANDROID__
        const String os = "android";
      #else
        const String os = "linux";
      #endif

      bool mac = false;
      bool ios = false;
      bool win = false;
      bool linux = true;

      #if defined(__unix__) || defined(unix) || defined(__unix)
        bool unix = true;
      #else
        bool unix = false;
      #endif

    #elif defined(__FreeBSD__)
      const String os = "freebsd";
      bool mac = false;
      bool ios = false;
      bool win = false;
      bool linux = false;

      #if defined(__unix__) || defined(unix) || defined(__unix)
        bool unix = true;
      #else
        bool unix = false;
      #endif

    #elif defined(BSD)
      const String os = "openbsd";
      bool ios = false;
      bool mac = false;
      bool win = false;
      bool linux = false;

      #if defined(__unix__) || defined(unix) || defined(__unix)
        bool unix = true;
      #else
        bool unix = false;
      #endif

    #endif
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

  inline uint64_t rand64 (void) {
    uint64_t r = 0;
    for (int i = 0; i < 64; i += RAND_MAX_WIDTH) {
      r <<= RAND_MAX_WIDTH;
      r ^= (unsigned) rand();
    }
    return r;
  }

  inline String getEnv (const char* variableName) {
    #if _WIN32
      char* variableValue = nullptr;
      std::size_t valueSize = 0;
      auto query = _dupenv_s(&variableValue, &valueSize, variableName);

      String result;
      if(query == 0 && variableValue != nullptr && valueSize > 0) {
        result.assign(variableValue, valueSize - 1);
        free(variableValue);
      }

      return result;
    #else
      auto v = getenv(variableName);

      if (v != nullptr) {
        return String(v);
      }

      return String("");
    #endif
  }

  inline auto setEnv (const char* s) {
    #if _WIN32
      return _putenv(s);
    #else

      return putenv((char*) &s[0]);
    #endif
  }

  inline void stdWrite (const String &str, bool isError) {
    #ifdef _WIN32
      StringStream ss;
      ss << str << std::endl;
      auto lineStr = ss.str();

      auto handle = isError ? STD_ERROR_HANDLE : STD_OUTPUT_HANDLE;
      WriteConsoleA(GetStdHandle(handle), lineStr.c_str(), lineStr.size(), NULL, NULL);
    #else
      (isError ? std::cerr : std::cout) << str << std::endl;
    #endif
  }

  #if !TARGET_OS_IPHONE && !TARGET_OS_SIMULATOR
    inline String readFile (std::filesystem::path path) {
      std::ifstream stream(path.c_str());
      String content;
      auto buffer = std::istreambuf_iterator<char>(stream);
      auto end = std::istreambuf_iterator<char>();
      content.assign(buffer, end);
      stream.close();
      return content;
    }

    inline void writeFile (std::filesystem::path path, String s) {
      std::ofstream stream(path.string());
      stream << s;
      stream.close();
    }

    inline void appendFile (std::filesystem::path path, String s) {
      std::ofstream stream;
      stream.open(path.string(), std::ios_base::app);
      stream << s;
      stream.close();
    }
  #endif


  //
  // All ipc uses a URI schema, so all ipc data needs to be
  // encoded as a URI component. This prevents escaping the
  // protocol.
  //
  const signed char HEX2DEC[256] = {
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

  inline String decodeURIComponent(const String& sSrc) {

    // Note from RFC1630:  "Sequences which start with a percent sign
    // but are not followed by two hexadecimal characters (0-9, A-F) are reserved
    // for future extension"

    auto s = replace(sSrc, "\\+", " ");
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

  const char SAFE[256] = {
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

#endif // SSC_COMMON_H
