#ifndef SSC_INTERNAL_APPLE_HH
#define SSC_INTERNAL_APPLE_HH

#include <objc/objc.h>
#include <objc/runtime.h>

#import <Webkit/Webkit.h>
#import <CoreFoundation/CoreFoundation.h>
#import <UserNotifications/UserNotifications.h>
#import <UniformTypeIdentifiers/UniformTypeIdentifiers.h>

#if TARGET_OS_IPHONE || TARGET_IPHONE_SIMULATOR
#import <UIKit/UIKit.h>
#else
#import <Cocoa/Cocoa.h>
#endif

#include "internal.hh"

using Task = id<WKURLSchemeTask>;
using Tasks = std::map<std::string, Task>;

@class BridgedWebView;
@interface SchemeHandler : NSObject<WKURLSchemeHandler>
@property (nonatomic) ssc::internal::Router* router;
-     (void) webView: (BridgedWebView*) webview
  startURLSchemeTask: (Task) task;
-    (void) webView: (BridgedWebView*) webview
  stopURLSchemeTask: (Task) task;
@end

@interface SchemeTasks : NSObject {
  std::unique_ptr<Tasks> tasks;
  std::recursive_mutex mutex;
}
- (Task) get: (std::string) id;
- (void) remove: (std::string) id;
- (bool) has: (std::string) id;
- (void) put: (std::string) id task: (Task) task;
@end

#endif
