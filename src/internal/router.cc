#include <stddef.h>

#include <map>
#include <mutex>
#include <string>

#if defined(__APPLE__)
  #include <objc/objc.h>
  #include <objc/runtime.h>
  #import <Webkit/Webkit.h>
  #import <Network/Network.h>
  #import <CoreFoundation/CoreFoundation.h>
  #import <UserNotifications/UserNotifications.h>
  #import <UniformTypeIdentifiers/UniformTypeIdentifiers.h>
#endif

#include "internal.hh"

using Lock = std::lock_guard<std::recursive_mutex>;
using Mutex = std::recursive_mutex;
using String = std::string;

#if defined(__APPLE__)
using Task = id<WKURLSchemeTask>;
using Tasks = std::map<String, Task>;

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
- (Task) get: (String) id;
- (void) remove: (String) id;
- (bool) has: (String) id;
- (void) put: (String) id task: (Task) task;
@end

@interface NetworkStatusObserver : NSObject
@property (strong, nonatomic) NSObject<OS_dispatch_queue>* monitorQueue;
@property (nonatomic) ssc::internal::Router* router;
@property (retain) nw_path_monitor_t monitor;
- (id) init;
@end
#endif

namespace ssc::internal {
  struct Router {
    #if defined(__APPLE__)
      NetworkStatusObserver* networkStatusObserver = nullptr;
      SchemeHandler* schemeHandler = nullptr;
      SchemeTasks* schemeTasks = nullptr;

    #endif
    std::function<void(String, String)> onNetworkStatusChangeCallback;

    void onNetworkStatusChange (String status, String message) {
      if (onNetworkStatusChangeCallback != nullptr) {
        this->onNetworkStatusChangeCallback(status, message);
      }
    }
  };

  void alloc (Router** router) {
    *router = new Router;
  }

  void init (Router* router, const RouterOptions& options) {
    #if defined(__APPLE__)
      router->networkStatusObserver = [NetworkStatusObserver new];
      router->schemeHandler = [SchemeHandler new];
      router->schemeTasks = [SchemeTasks new];

      [router->schemeHandler setRouter: router];
      [router->networkStatusObserver setRouter: router];
    #endif

    router->onNetworkStatusChangeCallback = options.onNetworkStatusChange;
  }

  void deinit (Router* router) {
    #if defined(__APPLE__)
      if (router->networkStatusObserver != nullptr) {
        #if !__has_feature(objc_arc)
          [router->networkStatusObserver release];
        #endif
      }

      if (router->schemeHandler != nullptr) {
        #if !__has_feature(objc_arc)
          [router->schemeHandler release];
        #endif
      }

      if (router->schemeTasks != nullptr) {
        #if !__has_feature(objc_arc)
        [router->schemeTasks release];
        #endif
      }

      router->networkStatusObserver = nullptr;
      router->schemeHandler = nullptr;
      router->schemeTasks = nullptr;
    #endif

    delete router;
  }

  static void registerSchemeHandler (Router *router) {
  #if defined(__linux__)
    // prevent this function from registering the `ipc://`
    // URI scheme handler twice
    static std::atomic<bool> registered = false;
    if (registered) return;
    registered = true;

    auto ctx = webkit_web_context_get_default();
    webkit_web_context_register_uri_scheme(ctx, "ipc", [](auto request, auto ptr) {
      auto uri = String(webkit_uri_scheme_request_get_uri(request));
      auto router = reinterpret_cast<Router *>(ptr);
      auto invoked = router->invoke(uri, [=](auto result, auto bytes, auto size) {
        auto size = bytes != nullptr ? size : result.size();
        auto body = bytes != nullptr ? bytes : result.c_str();

        auto freeFn = bytes != nullptr ? free : nullptr;
        auto stream = g_memory_input_stream_new_from_data(body, size, freeFn);
        auto response = webkit_uri_scheme_response_new(stream, size);

        webkit_uri_scheme_response_set_content_type(response, IPC_CONTENT_TYPE);
        webkit_uri_scheme_request_finish_with_response(request, response);
        g_object_unref(stream);
      });

      if (!invoked) {
        auto err = JSON::Object::Entries {
          {"source", uri},
          {"err", JSON::Object::Entries {
            {"message", "Not found"},
            {"type", "NotFoundError"},
            {"url", uri}
          }}
        };

        auto msg = JSON::Object(err).str();
        auto size = msg.size();
        auto bytes = msg.c_str();
        auto stream = g_memory_input_stream_new_from_data(bytes, size, 0);
        auto response = webkit_uri_scheme_response_new(stream, msg.size());

        webkit_uri_scheme_response_set_status(response, 404, "Not found");
        webkit_uri_scheme_response_set_content_type(response, IPC_CONTENT_TYPE);
        webkit_uri_scheme_request_finish_with_response(request, response);
        g_object_unref(stream);
      }
    },
    router,
    0);
  #endif
  }

  namespace router {
    bool send (
      Router* router,
      const String& seq,
      const String& data,
      const char* bytes,
      size_t size
    ) {
      #if defined(__APPLE__)
        if (seq.size() > 0 && seq != "-1" && [router->schemeTasks has: seq]) {
          auto task = [router->schemeTasks get: seq];
          auto msg = data;
          [router->schemeTasks remove: seq];

          #if !__has_feature(objc_arc)
            [task retain];
          #endif

          dispatch_async(dispatch_get_main_queue(), ^{
            auto headers = [[NSMutableDictionary alloc] init];
            auto length = bytes ? size : msg.size();

            headers[@"access-control-allow-origin"] = @"*";
            headers[@"access-control-allow-methods"] = @"*";
            headers[@"content-length"] = [@(length) stringValue];

            NSHTTPURLResponse *response = [[NSHTTPURLResponse alloc]
               initWithURL: task.request.URL
                statusCode: 200
               HTTPVersion: @"HTTP/1.1"
              headerFields: headers
            ];

            [task didReceiveResponse: response];

            if (bytes) {
              auto data = [NSData dataWithBytes: bytes length: size];
              [task didReceiveData: data];
            } else if (msg.size() > 0) {
              auto string = [NSString stringWithUTF8String: msg.c_str()];
              auto data = [string dataUsingEncoding: NSUTF8StringEncoding];
              [task didReceiveData: data];
            }

            [task didFinish];
            #if !__has_feature(objc_arc)
              [headers release];
              [response release];
            #endif
          });

          return true;
        }

      #endif

      return false;
    }
  }
}

#if defined(__APPLE__)
@implementation SchemeTasks
- (id) init {
  self = [super init];
  tasks  = std::unique_ptr<Tasks>(new Tasks());
  return self;
}

- (Task) get: (String) id {
  Lock lock(mutex);
  if (tasks->find(id) == tasks->end()) return Task{};
  return tasks->at(id);
}

- (bool) has: (String) id {
  Lock lock(mutex);
  if (id.size() == 0) return false;
  return tasks->find(id) != tasks->end();
}

- (void) remove: (String) id {
  Lock lock(mutex);
  if (tasks->find(id) == tasks->end()) return;
  tasks->erase(id);
}

- (void) put: (String) id task: (Task) task {
  Lock lock(mutex);
  tasks->insert_or_assign(id, task);
}
@end

@implementation NetworkStatusObserver
- (id) init {
  self = [super init];
  dispatch_queue_attr_t attrs = dispatch_queue_attr_make_with_qos_class(
    DISPATCH_QUEUE_SERIAL,
    QOS_CLASS_UTILITY,
    DISPATCH_QUEUE_PRIORITY_DEFAULT
  );

  _monitorQueue = dispatch_queue_create(
    "co.socketsupply.queue.ipc.network-status-observer",
    attrs
  );

  // self.monitor = nw_path_monitor_create_with_type(nw_interface_type_wifi);
  _monitor = nw_path_monitor_create();
  nw_path_monitor_set_queue(self.monitor, self.monitorQueue);
  nw_path_monitor_set_update_handler(self.monitor, ^(nw_path_t path) {
    nw_path_status_t status = nw_path_get_status(path);

    String statusName;
    String message;

    switch (status) {
      case nw_path_status_invalid: {
        statusName = "offline";
        message = "Network path is invalid";
        break;
      }
      case nw_path_status_satisfied: {
        statusName = "online";
        message = "Network is usable";
        break;
      }
      case nw_path_status_satisfiable: {
        statusName = "online";
        message = "Network may be usable";
        break;
      }
      case nw_path_status_unsatisfied: {
        statusName = "offline";
        message = "Network is not usable";
        break;
      }
    }

    dispatch_async(dispatch_get_main_queue(), ^{
      self.router->onNetworkStatusChange(statusName, message);
    });
  });

  nw_path_monitor_start(self.monitor);

  return self;
}

- (void) userNotificationCenter: (UNUserNotificationCenter *) center
        willPresentNotification: (UNNotification *) notification
          withCompletionHandler: (void (^)(UNNotificationPresentationOptions options)) completionHandler
{
  completionHandler(UNNotificationPresentationOptionList | UNNotificationPresentationOptionBanner);
}
@end
#endif
