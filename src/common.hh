#ifndef SSC_COMMON_H
#define SSC_COMMON_H

/*
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
*/

#ifndef DEBUG
#define DEBUG 0
#endif

#ifndef SSC_SETTINGS
#define SSC_SETTINGS ""
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

#endif // SSC_COMMON_H
