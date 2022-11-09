#ifndef SSC_CORE_PLATFORM_HEADERS_HH
#define SSC_CORE_PLATFORM_HEADERS_HH

#if !defined(SSC_INLINE_INCLUDE)
  #include <errno.h>
  #include <math.h>
  #include <stddef.h>
  #include <stdio.h>

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
    #include <CoreFoundation/CoreFoundation.h>
    #include <TargetConditionals.h>
    #if TARGET_OS_IPHONE || TARGET_IPHONE_SIMULATOR
      #include <_types/_uint64_t.h>
      #include <netinet/in.h>
      #include <sys/un.h>
    #else
      #include <objc/objc-runtime.h>
    #endif
  #endif

  #include <any>
  #include <algorithm>
  #include <functional>
  #include <map>
  #include <mutex>
  #include <queue>
  #include <regex>
  #include <semaphore>
  #include <string>
  #include <sstream>
  #include <thread>
  #include <vector>

  /* TODO
  #include <array>
  #include <chrono>
  #include <cstdint>
  #include <iostream>
  #include <filesystem>
  #include <fstream>
  #include <span>
  */

  #include "common.hh"
#endif

#endif
