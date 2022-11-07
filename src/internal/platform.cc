#include <string>

#if defined(__APPLE__)
  #import <Foundation/Foundation.h>
  #import <UserNotifications/UserNotifications.h>
  #if TARGET_OS_IPHONE || TARGET_IPHONE_SIMULATOR
    #import <UIKit/UIKit.h>
  #else
    #import <Cocoa/Cocoa.h>
  #endif
#endif

#include "internal.hh"

using String = std::string;

namespace ssc::internal {
  struct Platform {
  };

  void alloc (Platform** platform) {
    *platform = new Platform;
  }

  void init (Platform* platform) {
  }

  void deinit (Platform* platform) {
    delete platform;
  }
}

namespace ssc::internal::platform {
  void notify (
    Platform* platform,
    const String& title,
    const String& body,
    NotifyCallback cb
  ) {
    #if defined(__APPLE__)
    auto center = [UNUserNotificationCenter currentNotificationCenter];
    auto content = [[UNMutableNotificationContent alloc] init];
    content.body = [NSString stringWithUTF8String: body.c_str()];
    content.title = [NSString stringWithUTF8String: title.c_str()];
    content.sound = [UNNotificationSound defaultSound];

    auto trigger = [UNTimeIntervalNotificationTrigger
      triggerWithTimeInterval: 1.0f
                      repeats: NO
    ];

    auto request = [UNNotificationRequest
      requestWithIdentifier: @"LocalNotification"
                    content: content
                    trigger: trigger
    ];

    auto options = (
      UNAuthorizationOptionAlert |
      UNAuthorizationOptionBadge |
      UNAuthorizationOptionSound
    );

    [center requestAuthorizationWithOptions: options
                          completionHandler: ^(BOOL granted, NSError* error)
    {
      #if !__has_feature(objc_arc)
      [content release];
      [trigger release];
      #endif

      if (granted) {
        cb(nullptr);
      } else if (error) {
        [center addNotificationRequest: request withCompletionHandler: ^(NSError* error) {
          #if !__has_feature(objc_arc)
          [request release];
          #endif

          if (error) {
            cb([error.debugDescription UTF8String]);
            // debug("Unable to create notification: %@", error.debugDescription);
          } else {
            cb(nullptr);
          }
        }];
      } else {
        cb("Failed to create notification");
      }

      if (!error || granted) {
        #if !__has_feature(objc_arc)
        [request release];
        #endif
      }
    }];
    #endif
  }

  void openExternal (
    Platform*,
    const std::string& value,
    OpenExternalCallback cb
  ) {
    #if defined(__APPLE__)
      auto string = [NSString stringWithUTF8String: value.c_str()];
      auto url = [NSURL URLWithString: string];

      #if TARGET_OS_IPHONE || TARGET_IPHONE_SIMULATOR
        auto app = [UIApplication sharedApplication];
        [app openURL: url options: @{} completionHandler: ^(BOOL success) {
          if (!success) {
            cb("Failed to open external URL");
          } else {
            cb(nullptr);
          }
        }];
      #else
        auto workspace = [NSWorkspace sharedWorkspace];
        auto configuration = [NSWorkspaceOpenConfiguration configuration];
        [workspace openURL: url
             configuration: configuration
         completionHandler: ^(NSRunningApplication *app, NSError *error) {
          if (error) {
             cb([error.debugDescription UTF8String]);
           } else {
             cb(nullptr);
           }
          [configuration release];
        }];
      #endif
    #endif
  }
}
