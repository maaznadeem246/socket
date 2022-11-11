#include <stddef.h>

#include <map>
#include <mutex>
#include <string>

#if defined(__APPLE__)
#include "apple.hh"
#endif

#include "internal.hh"

using ssc::internal::Lock;
using ssc::internal::String;
using ssc::internal::Vector;

namespace ssc::internal {
  struct Router {
    #if defined(__APPLE__)
      SchemeHandler* schemeHandler = nullptr;
      SchemeTasks* schemeTasks = nullptr;

    #endif

    router::NetworkStatusChangeCallback onNetworkStatusChangeCallback;
    router::InvokeCallback onInvokeCallback;

    void onNetworkStatusChange (String status, String message) {
      if (this->onNetworkStatusChangeCallback != nullptr) {
        this->onNetworkStatusChangeCallback(status, message);
      }
    }

    bool onInvoke (String uri, router::InvokeResultCallback cb) {
      if (this->onInvokeCallback != nullptr) {
        return this->onInvokeCallback(uri, cb);
      }

      return false;
    }
  };

  void alloc (Router** router) {
    *router = new Router;
  }

  void init (Router* router, const router::Options& options) {
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

    bool invoke (Router* router, String uri, InvokeResultCallback cb) {
      return router->onInvoke(uri, cb);
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


@class SSCBridgedWebView;
@implementation SchemeHandler
- (void) webView: (SSCBridgedWebView*) webview stopURLSchemeTask: (Task) task {}
- (void) webView: (SSCBridgedWebView*) webview startURLSchemeTask: (Task) task {
  auto url = String(task.request.URL.absoluteString.UTF8String);
  auto message = Message {url};
  auto seq = message.seq;

  if (String(task.request.HTTPMethod.UTF8String) == "OPTIONS") {
    auto headers = [NSMutableDictionary dictionary];

    headers[@"access-control-allow-origin"] = @"*";
    headers[@"access-control-allow-methods"] = @"*";

    auto response = [[NSHTTPURLResponse alloc]
      initWithURL: task.request.URL
       statusCode: 200
      HTTPVersion: @"HTTP/1.1"
     headerFields: headers
    ];

    [task didReceiveResponse: response];
    [task didFinish];
    #if !__has_feature(objc_arc)
    [response release];
    #endif

    return;
  }

  if (message.name == "post") {
    auto headers = [NSMutableDictionary dictionary];
    auto id = std::stoull(message.get("id"));
    auto post = self.router->core->getPost(id);

    headers[@"access-control-allow-origin"] = @"*";
    headers[@"content-length"] = [@(post.length) stringValue];

    if (post.headers.size() > 0) {
      auto lines = ssc::split(ssc::trim(post.headers), '\n');

      for (auto& line : lines) {
        auto pair = split(trim(line), ':');
        auto key = [NSString stringWithUTF8String: trim(pair[0]).c_str()];
        auto value = [NSString stringWithUTF8String: trim(pair[1]).c_str()];
        headers[key] = value;
      }
    }

    NSHTTPURLResponse *response = [[NSHTTPURLResponse alloc]
       initWithURL: task.request.URL
        statusCode: 200
       HTTPVersion: @"HTTP/1.1"
      headerFields: headers
    ];

    [task didReceiveResponse: response];

    if (post.body) {
      auto data = [NSData dataWithBytes: post.body length: post.length];
      [task didReceiveData: data];
    } else {
      auto string = [NSString stringWithUTF8String: ""];
      auto data = [string dataUsingEncoding: NSUTF8StringEncoding];
      [task didReceiveData: data];
    }

    [task didFinish];
    #if !__has_feature(objc_arc)
    [response release];
    #endif

    // 16ms timeout before removing post and potentially freeing `post.body`
    NSTimeInterval timeout = 0.16;
    auto block = ^(NSTimer* timer) {
      dispatch_async(dispatch_get_main_queue(), ^{
        self.router->core->removePost(id);
      });
    };

    [NSTimer timerWithTimeInterval: timeout repeats: NO block: block ];

    return;
  }

  size_t bufsize = 0;
  char *body = NULL;

  if (seq.size() > 0 && seq != "-1") {
    #if !__has_feature(objc_arc)
    [task retain];
    #endif

    [self.router->schemeTasks put: seq task: task];
  }

  // if there is a body on the reuqest, pass it into the method router.
  auto rawBody = task.request.HTTPBody;

  if (rawBody) {
    const void* data = [rawBody bytes];
    bufsize = [rawBody length];
    body = (char *) data;
  }

  if (!self.router->invoke(url, body, bufsize)) {
    NSMutableDictionary* headers = [NSMutableDictionary dictionary];
    auto json = JSON::Object::Entries {
      {"err", JSON::Object::Entries {
        {"message", "Not found"},
        {"type", "NotFoundError"},
        {"url", url}
      }}
    };

    auto msg = JSON::Object(json).str();
    auto str = [NSString stringWithUTF8String: msg.c_str()];
    auto data = [str dataUsingEncoding: NSUTF8StringEncoding];

    headers[@"access-control-allow-origin"] = @"*";
    headers[@"content-length"] = [@(msg.size()) stringValue];

    NSHTTPURLResponse *response = [[NSHTTPURLResponse alloc]
       initWithURL: task.request.URL
        statusCode: 404
       HTTPVersion: @"HTTP/1.1"
      headerFields: headers
    ];

    [task didReceiveResponse: response];
    [task didReceiveData: data];
    [task didFinish];
    #if !__has_feature(objc_arc)
    [response release];
    #endif
  }
}
@end
