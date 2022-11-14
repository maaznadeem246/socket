#include "platform.hh"
#include "private.hh"

namespace ssc::core::platform {
  void CorePlatform::notify (
    const String& title,
    const String& body,
    NotifyCallback callback
  ) const {
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
        callback(nullptr);
      } else if (error) {
        [center addNotificationRequest: request withCompletionHandler: ^(NSError* error) {
          #if !__has_feature(objc_arc)
          [request release];
          #endif

          if (error) {
            callback([error.debugDescription UTF8String]);
            // debug("Unable to create notification: %@", error.debugDescription);
          } else {
            callback(nullptr);
          }
        }];
      } else {
        callback("Failed to create notification");
      }

      if (!error || granted) {
        #if !__has_feature(objc_arc)
        [request release];
        #endif
      }
    }];
    #endif
  }

  void CorePlatform::openExternal (
    const String& value,
    OpenExternalCallback callback
  ) const {
    #if defined(__APPLE__)
      auto string = [NSString stringWithUTF8String: value.c_str()];
      auto url = [NSURL URLWithString: string];

      #if TARGET_OS_IPHONE || TARGET_IPHONE_SIMULATOR
        auto app = [UIApplication sharedApplication];
        [app openURL: url options: @{} completionHandler: ^(BOOL success) {
          if (!success) {
            callback("Failed to open external URL");
          } else {
            callback(nullptr);
          }
        }];
      #else
        auto workspace = [NSWorkspace sharedWorkspace];
        auto configuration = [NSWorkspaceOpenConfiguration configuration];
        [workspace openURL: url
             configuration: configuration
         completionHandler: ^(NSRunningApplication *app, NSError *error) {
          if (error) {
             callback([error.debugDescription UTF8String]);
           } else {
             callback(nullptr);
           }
          [configuration release];
        }];
      #endif
    #endif
  }
}
