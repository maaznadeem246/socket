#ifndef SSC_PLATFORM_H
#define SSC_PLATFORM_H

  #if defined(_WIN32)
    #undef _WINSOCKAPI_
    #define _WINSOCKAPI_
  #endif

  #if defined(__APPLE__)
    #import <CoreFoundation/CoreFoundation.h>
/*
      #import <Foundation/Foundation.h>
      #import <CoreBluetooth/CoreBluetooth.h>
      #import <UserNotifications/UserNotifications.h>
      #import <UniformTypeIdentifiers/UniformTypeIdentifiers.h>

      #if TARGET_OS_IPHONE || TARGET_IPHONE_SIMULATOR
        #import <UIKit/UIKit.h>
      #else
        #import <Cocoa/Cocoa.h>
      #endif
      */
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

  #include "common.hh"
#endif
