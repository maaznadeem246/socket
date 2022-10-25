module;

#include "../platform.hh"

import :interfaces;
import :json;

export module ssc.runtime:platform;

export namespace ssc {
  class Platform : public Module {
    public:
      Platform (auto runtime) : Module(runtime) {}
      void event (
        const String seq,
        const String event,
        const String data,
        Module::Callback cb
      );
      void notify (
        const String seq,
        const String title,
        const String body,
        Module::Callback cb
      );
      void openExternal (
        const String seq,
        const String value,
        Module::Callback cb
      );
  };

  void Platform::event (
    const String seq,
    const String event,
    const String data,
    Module::Callback cb
  ) {
    this->runtime->dispatchEventLoop([=, this]() {
      // init page
      if (event == "domcontentloaded") {
        Lock lock(this->runtime->fs.mutex);

        for (auto const &tuple : this->runtime->fs.descriptors) {
          auto desc = tuple.second;
          if (desc != nullptr) {
            Lock lock(desc->mutex);
            desc->stale = true;
          } else {
            this->runtime->fs.descriptors.erase(tuple.first);
          }
        }
      }

      auto json = JSON::Object::Entries {
        {"source", "event"},
        {"data", JSON::Object::Entries{}}
      };

      cb(seq, json, Post{});
    });
  }

  void Platform::notify (
    const String seq,
    const String title,
    const String body,
    Module::Callback cb
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
        auto json = JSON::Object::Entries {
          {"source", "platform.notify"},
          {"data", JSON::Object::Entries {}}
        };

        cb(seq, json, Post{});
      } else if (error) {
        [center addNotificationRequest: request
                 withCompletionHandler: ^(NSError* error)
        {
          auto json = JSON::Object {};

          #if !__has_feature(objc_arc)
          [request release];
          #endif

          if (error) {
            json = JSON::Object::Entries {
              {"source", "platform.notify"},
              {"err", JSON::Object::Entries {
                {"message", [error.debugDescription UTF8String]}
              }}
            };

            debug("Unable to create notification: %@", error.debugDescription);
          } else {
            json = JSON::Object::Entries {
              {"source", "platform.notify"},
              {"data", JSON::Object::Entries {}}
            };
          }

         cb(seq, json, Post{});
        }];
      } else {
        auto json = JSON::Object::Entries {
          {"source", "platform.notify"},
          {"err", JSON::Object::Entries {
            {"message", "Failed to create notification"}
          }}
        };

        cb(seq, json, Post{});
      }

      if (!error || granted) {
        #if !__has_feature(objc_arc)
        [request release];
        #endif
      }
    }];
#endif
  }

  void Platform::openExternal (
    const String seq,
    const String value,
    Module::Callback cb
  ) {
#if defined(__APPLE__)
    auto string = [NSString stringWithUTF8String: value.c_str()];
    auto url = [NSURL URLWithString: string];

    #if TARGET_OS_IPHONE || TARGET_IPHONE_SIMULATOR
    auto app = [UIApplication sharedApplication];
    [app openURL: url options: @{} completionHandler: ^(BOOL success) {
      auto json = JSON::Object {};

      if (!success) {
        json = JSON::Object::Entries {
          {"source", "platform.notify"},
          {"err", JSON::Object::Entries {
            {"message", "Failed to open external URL"}
          }}
        };
      } else {
        json = JSON::Object::Entries {
          {"source", "platform.notify"},
          {"data", JSON::Object::Entries {}}
        };
      }

      cb(seq, json, Post{});
    }];
    #else
    auto workspace = [NSWorkspace sharedWorkspace];
    auto configuration = [NSWorkspaceOpenConfiguration configuration];
    [workspace openURL: url
         configuration: configuration
     completionHandler: ^(NSRunningApplication *app, NSError *error)
    {
       auto json = JSON::Object {};
       if (error) {
         json = JSON::Object::Entries {
           {"source", "platform.openExternal"},
           {"err", JSON::Object::Entries {
             {"message", [error.debugDescription UTF8String]}
           }}
         };
       } else {
        json = JSON::Object::Entries {
          {"source", "platform.openExternal"},
          {"data", JSON::Object::Entries {}}
        };
       }

      cb(seq, json, Post{});

      [configuration release];
    }];
    #endif
#endif
  }
}
