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

    [center
      requestAuthorizationWithOptions: options
                    completionHandler: ^(BOOL granted, NSError* error) {
      #if !__has_feature(objc_arc)
      [content release];
      [trigger release];
      #endif

      if (granted) {
        callback("");
      } else if (!error) {
        callback("Failed to create notification");
      } else {
        [center addNotificationRequest: request withCompletionHandler: ^(NSError* error) {
          #if !__has_feature(objc_arc)
          [request release];
          #endif

          if (error) {
            callback([error.debugDescription UTF8String]);
            // debug("Unable to create notification: %@", error.debugDescription);
          } else {
            callback("");
          }
        }];
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
            callback("");
          }
        }];
      #else
        auto workspace = [NSWorkspace sharedWorkspace];
        auto configuration = [NSWorkspaceOpenConfiguration configuration];
        [workspace openURL: url
             configuration: configuration
         completionHandler: ^(NSRunningApplication *app, NSError *error)
         {
          if (error) {
             callback([error.debugDescription UTF8String]);
           } else {
             callback("");
           }
          #if !__has_feature(objc_arc)
            [configuration release];
          #endif
        }];
      #endif
    #endif
  }

  void CorePlatform::cwd (CWDCallback callback) const {
    String cwd = "";
  #if defined(__linux__)
    auto canonical = fs::canonical("/proc/self/exe");
    cwd = fs::path(canonical).parent_path().string();
  #elif defined(__APPLE__)
    auto fileManager = [NSFileManager defaultManager];
    auto currentDirectoryPath = [fileManager currentDirectoryPath];
    auto currentDirectory = [NSHomeDirectory() stringByAppendingPathComponent: currentDirectoryPath];
    cwd = String([currentDirectory UTF8String]);
  #elif defined(_WIN32)
    wchar_t filename[MAX_PATH];
    GetModuleFileNameW(NULL, filename, MAX_PATH);
    auto path = fs::path { filename }.remove_filename();
    cwd = path.string();
  #endif

    callback(cwd);
  }
}
