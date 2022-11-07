#ifndef SSC_WINDOW_APPLE_HH
#define SSC_WINDOW_APPLE_HH

#import <Foundation/Foundation.h>
#import <Webkit/Webkit.h>
#import <Network/Network.h>
#import <UserNotifications/UserNotifications.h>
#import <UniformTypeIdentifiers/UniformTypeIdentifiers.h>

#if TARGET_OS_IPHONE || TARGET_IPHONE_SIMULATOR
#import <UIKit/UIKit.h>
#else
#import <Cocoa/Cocoa.h>
#endif

#include <map>

using Task = id<WKURLSchemeTask>;
using Tasks = std::map<std::string, Task>;


#endif
