#ifndef SSC_SOCKET_PLATFORM_HH
#define SSC_SOCKET_PLATFORM_HH

#if !defined(SSC_INLINE_INCLUDE)
  #include <errno.h>
  #include <stddef.h>
  #include <stdio.h>
  #include <stdlib.h>

  #if defined(_WIN32)
    #if !defined(WIN32_LEAN_AND_MEAN)
      #define WIN32_LEAN_AND_MEAN
    #endif

    #include <dwmapi.h>
    #include <io.h>
    #include <tchar.h>
    #include <shlobj_core.h>
    #include <shobjidl.h>
    #include <signal.h>
    #include <wingdi.h>
    #include <Winbase.h>
    #include <windows.h>
    #include <ws2tcpip.h>

    #include <future>
  #else
    #include <arpa/inet.h>
    #include <ifaddrs.h>
    #include <sys/socket.h>
    #include <sys/types.h>
    #include <sys/wait.h>
    #include <unistd.h>
  #endif

  #if defined(__APPLE__)
    #include <TargetConditionals.h>
    #if TARGET_OS_IPHONE || TARGET_IPHONE_SIMULATOR
      #include <_types/_uint64_t.h>
      #include <netinet/in.h>
      #include <sys/un.h>
    #endif
  #endif
#endif

#include <exception>
#include <string>
#include "common.hh"

namespace ssc::platform {
  struct PlatformInfo {
    #if defined(__x86_64__) || defined(_M_X64)
      const std::string arch = "x86_64";
    #elif defined(__aarch64__) || defined(_M_ARM64)
      const std::string arch = "arm64";
    #else
      const std::string arch = "unknown";
    #endif

    #if defined(_WIN32)
      const std::string os = "win32";
      bool mac = false;
      bool ios = false;
      bool win = true;
      bool linux = false;
      bool unix = false;
    #elif defined(__APPLE__)
      bool win = false;
      bool linux = false;
      #if TARGET_OS_IPHONE || TARGET_IPHONE_SIMULATOR
        std::string os = "ios";
        bool ios = true;
        bool mac = false;
      #else
        std::string os = "mac";
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
        const std::string os = "android";
      #else
        const std::string os = "linux";
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
      const std::string os = "freebsd";
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
      const std::string os = "openbsd";
      bool ios = false;
      bool mac = false;
      bool win = false;
      #if defined(__unix__) || defined(unix) || defined(__unix)
        bool unix = true;
      #else
        bool unix = false;
      #endif
    #endif
  };
}
#endif
