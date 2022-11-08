#ifndef SSC_CORE_PRIVATE_H
#define SSC_CORE_PRIVATE_H
  #if defined(_WIN32)
    #undef _WINSOCKAPI_
    #define _WINSOCKAPI_
  #endif

  #include "common.hh"

  #if defined(__APPLE__)
    #include <Foundation/Foundation.h>
    #include <CoreBluetooth/CoreBluetooth.h>
    #include <UserNotifications/UserNotifications.h>
    #include <UniformTypeIdentifiers/UniformTypeIdentifiers.h>
    #if TARGET_OS_IPHONE || TARGET_IPHONE_SIMULATOR
      #include <UIKit/UIKit.h>
    #else
      #include <Cocoa/Cocoa.h>
    #endif
  #elif defined(__linux__) && !defined(__ANDROID__)
    #include <JavaScriptCore/JavaScript.h>
    #include <webkit2/webkit2.h>
    #include <gtk/gtk.h>
  #elif defined(_WIN32)
    #include <WebView2.h>
    #include <WebView2Experimental.h>
    #include <WebView2EnvironmentOptions.h>
    #include <WebView2ExperimentalEnvironmentOptions.h>
    #pragma comment(lib, "advapi32.lib")
    #pragma comment(lib, "shell32.lib")
    #pragma comment(lib, "version.lib")
    #pragma comment(lib, "user32.lib")
    #pragma comment(lib, "WebView2LoaderStatic.lib")
    #pragma comment(lib, "uv_a.lib")
  #endif
#endif
