#ifndef SSC_SOCKET_COMMON_H
#define SSC_SOCKET_COMMON_H

#define _STR_VALUE(arg) #arg
#define STR_VALUE(arg) _STR_VALUE(arg)

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

#if !DEBUG
  #if defined(debug)
    #undef debug
  #endif
  #define debug(format, ...)
#endif

// macOS/iOS
#if defined(__APPLE__)
  #if !defined(debug)
    #define debug(format, ...) fprintf(stderr, format "\n", ##__VA_ARGS__)
  #endif
#endif

// Linux
#if defined(__linux__) && !defined(__ANDROID__)
  #if !defined(debug)
    #define debug(format, ...) fprintf(stderr, format "\n", ##__VA_ARGS__)
  #endif
#endif

// Android (Linux)
#if defined(__linux__) && defined(__ANDROID__)
  #if !defined(debug)
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
  #define isatty _isatty
  #define fileno _fileno
  #if !defined(debug)
    #define debug(format, ...) fprintf(stderr, format "\n", ##__VA_ARGS__)
  #endif
#endif

#endif
