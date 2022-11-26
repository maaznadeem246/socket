#include <socket/runtime.hh>

namespace ssc::runtime {
  Platform::Platform (Runtime* runtime)
  : loop(runtime->loops.main)
  {
    // noop
  }

  void Platform::notify (
    const String seq,
    const String title,
    const String body,
    Callback callback
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
                    completionHandler: ^(BOOL granted, NSError* error)
    {
      auto json = JSON::Object();
      json["source"] = "platform.notify";
      #if !__has_feature(objc_arc)
      [content release];
      [trigger release];
      #endif

      if (granted) {
        callback(seq, json, Data{});
      } else if (!error) {
        json["err"] = JSON::Object::Entries {
          {"message", "Failed to create notification"}
        };
        callback(seq, json, Data{});
      } else {
        [center addNotificationRequest: request withCompletionHandler: ^(NSError* error) {
          #if !__has_feature(objc_arc)
          [request release];
          #endif

          if (error) {
            json["err"] = JSON::Object::Entries {
              {"message", [error.debugDescription UTF8String]}
            };
            // debug("Unable to create notification: %@", error.debugDescription);
          }

          callback(seq, json, Data{});
        }];
      }

      if (!error || granted) {
        #if !__has_feature(objc_arc)
        [request release];
        #endif
      }
    }];
    #else
    // TODO: unsupported
    #endif
  }

  void Platform::openExternal (
    const String seq,
    const String value,
    Callback callback
  ) const {
    #if defined(__APPLE__)
      auto string = [NSString stringWithUTF8String: value.c_str()];
      auto url = [NSURL URLWithString: string];
      auto json = JSON::Object();
      json["source"] = "platform.openExternal";

      #if TARGET_OS_IPHONE || TARGET_IPHONE_SIMULATOR
        auto app = [UIApplication sharedApplication];
        [app openURL: url options: @{} completionHandler: ^(BOOL success) {
          if (!success) {
            json["err"] = JSON::Object::Entries {
              {"message", "Failed to open external URL"}
            };
          }

          callback(seq, json, Data{});
        }];
      #else
        auto workspace = [NSWorkspace sharedWorkspace];
        auto configuration = [NSWorkspaceOpenConfiguration configuration];
        [workspace openURL: url
             configuration: configuration
         completionHandler: ^(NSRunningApplication *app, NSError *error)
        {
          if (error) {
            json["err"] = JSON::Object::Entries {
              {"message", [error.debugDescription UTF8String]}
            };
           }

          callback(seq, json, Data{});
          #if !__has_feature(objc_arc)
            [configuration release];
          #endif
        }];
      #endif
    #endif
  }

  void Platform::cwd (const String seq, Callback callback) const {
    JSON::Object json;
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

    if (cwd.size() == 0) {
      json = JSON::Object::Entries {
        {"source", "platform.cwd"},
        {"err", JSON::Object::Entries {
          {"message", "Could not determine current working directory"}
        }}
      };
    } else {
      json = JSON::Object::Entries {
        {"source", "platform.cwd"},
        {"data", cwd}
      };
    }

    callback(seq, json, Data{});
  }

  void Platform::event (
    const String seq,
    const String event,
    const String data,
    Callback callback
  ) {
    this->loop.dispatch([=, this]() {
      /* FIXIME
      auto fs = this->runtime->interfaces.fs.get().as<FS>();
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

      auto json = JSON::Object::Entries {
        {"source", "platform.event"},
        {"data", JSON::Object::Entries{}}
      };

      callback(seq, json, Data{});
    */
    });
  }
}
