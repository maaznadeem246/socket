#ifndef SSC_SOCKET_PLATFORM_HH
#define SSC_SOCKET_PLATFORM_HH

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
#endif

#if defined(__APPLE__)
  #include <CoreBluetooth/CoreBluetooth.h>
  #include <Foundation/Foundation.h>
  #include <TargetConditionals.h>
  #include <UniformTypeIdentifiers/UniformTypeIdentifiers.h>
  #include <UserNotifications/UserNotifications.h>
  #include <WebKit/WebKit.h>

  #if TARGET_OS_IPHONE || TARGET_IPHONE_SIMULATOR
    #include <_types/_uint64_t.h>
    #include <netinet/in.h>
    #include <sys/un.h>
    #include <UIKit/UIKit.h>
    #include <objc/runtime.h>
  #elif TARGET_OS_MAC
    #include <objc/objc-runtime.h>
    #include <Cocoa/Cocoa.h>
  #endif
#endif

#if defined(__linux__) && !defined(__ANDROID__)
  #include <JavaScriptCore/JavaScript.h>
  #include <webkit2/webkit2.h>
  #include <gtk/gtk.h>
#endif

#if !defined(_WIN32)
  #include <arpa/inet.h>
  #include <ifaddrs.h>
  #include <sys/socket.h>
  #include <sys/types.h>
  #include <sys/wait.h>
  #include <unistd.h>
#endif


#include "common.hh"

namespace ssc {
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
      #if TARGET_OS_IPHONE || TARGET_IPHONE_SIMULATOR
        // eventually, we'll support tvos (TARGET_OS_TV) here too
        std::string os = "ios";
        bool ios = true;
        bool mac = false;
      #elif TARGET_OS_MAC
        std::string os = "mac";
        bool ios = false;
        bool mac = true;
      #endif
      bool win = false;
      bool linux = false;
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
