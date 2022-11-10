#include "../private.hh"
#include "webview.hh"

using ssc::internal::webview::Lock;
using ssc::internal::webview::Mutex;
using ssc::internal::webview::String;
using ssc::internal::webview::Message;

#if defined(__APPLE__)
using SchemeTask = id<WKURLSchemeTask>;
using SchemeTasks = std::map<std::string, SchemeTask>;

class SchemeTaskManager {
  public:
    SchemeTasks tasks;
    Mutex mutex;

    SchemeTask get (String id) {
      Lock lock(this->mutex);
      if (this->tasks.find(id) == this->tasks.end()) {
        return SchemeTask{};
      }

      return this->tasks.at(id);
    }

    bool has (String id) {
      Lock lock(this->mutex);
      return id.size() > 0 && this->tasks.find(id) != this->tasks.end();
    }

    void remove (String id) {
      Lock lock(this->mutex);
      if (this->has(id)) {
        this->tasks.erase(id);
      }
    }

    void put (String id, SchemeTask task) {
      Lock lock(this->mutex);
      this->tasks.insert_or_assign(id, task);
    }
};

@interface SchemeHandler : NSObject<WKURLSchemeHandler>
@property (nonatomic) ssc::internal::webview::SchemeHandler* handler;
@property (nonatomic) SchemeTaskManager* taskManager;
- (void) webView: (WKWebView*) webview startURLSchemeTask: (SchemeTask) task;
- (void) webView: (WKWebView*) webview stopURLSchemeTask: (SchemeTask) task;
@end
#endif

namespace ssc::internal::webview {
  SchemeHandler::SchemeHandler (
    const String& scheme,
    DataManager* dataManager,
    SchemeRequestCallback onSchemeRequestCallback
  ) {
    this->scheme = scheme;
    this->dataManager = dataManager;
    this->onSchemeRequestCallback = onSchemeRequestCallback;

    #if defined(__APPLE__)
      auto schemeHandler = [::SchemeHandler new];
      schemeHandler.handler = this;
      schemeHandler.taskManager = new SchemeTaskManager();
      this->internal = (void*) schemeHandler;
    #elif defined(__linux__) && !defined(__ANDROID__)
    #endif
  }

  SchemeHandler::~SchemeHandler () {
    #if defined(__APPLE__)
      if (this->internal != nullptr) {
        auto schemeHandler = (::SchemeHandler*) this->internal;
        delete schemeHandler.taskManager;
        [schemeHandler release];
        this->internal = nullptr;
      }
    #endif
  }

  void SchemeHandler::onSchemeRequest (const SchemeRequest request) {
    if (this->onSchemeRequestCallback != nullptr) {
      this->onSchemeRequestCallback(request);
    }
  }

  void SchemeRequest::end (
    const SchemeResponseStatusCode statusCode,
    const Headers headers,
    const char* bytes,
    size_t size
  ) const {
    return this->end(SchemeResponse {
      .statusCode = statusCode,
      .headers = headers,
      .body = { .bytes = (char*) bytes, .size = size }
    });
  }

  void SchemeRequest::end (const SchemeResponse& response) const {
    #if defined(__APPLE__)
      auto json = response.body.json.str();
      auto task = (SchemeTask) this->internal;
      auto headerFields = [NSMutableDictionary dictionary];

      for (const auto& header : response.headers.entries) {
        auto key = [NSString stringWithUTF8String: trim(header.key).c_str()];
        auto value = [NSString stringWithUTF8String: trim(header.value.str()).c_str()];
        headerFields[key] = value;
      }

      if (json.size() > 0) {
        headerFields[@"content-length"] = [@(json.size()) stringValue];
      } else if (response.body.size > 0) {
        headerFields[@"content-length"] = [@(response.body.size) stringValue];
      }

      auto taskResponse = [[NSHTTPURLResponse alloc]
         initWithURL: task.request.URL
          statusCode: response.statusCode
         HTTPVersion: @"HTTP/1.1"
        headerFields: headerFields
      ];

      [task didReceiveResponse: taskResponse];

      if (response.body.bytes) {
        auto data = [NSData
          dataWithBytes: response.body.bytes
                 length: response.body.size
        ];

        [task didReceiveData: data];
      } else {
        auto string = [NSString stringWithUTF8String: json.c_str()];
        auto data = [string dataUsingEncoding: NSUTF8StringEncoding];
        [task didReceiveData: data];
      }

      [task didFinish];
      #if !__has_feature(objc_arc)
        [taskResponse release];
      #endif
    #endif
  }

  IPCSchemeHandler::IPCSchemeHandler (
    DataManager* dataManager,
    IPCSchemeRequestRouteCallback onIPCSchemeRequestRouteCallback
  ) : SchemeHandler("ipc", dataManager, nullptr) {
    this->onIPCSchemeRequestRouteCallback = onIPCSchemeRequestRouteCallback;
  }

  void IPCSchemeHandler::onSchemeRequest (const SchemeRequest request) {
    auto response = SchemeResponse { &request };
    auto message = Message(request.url);

    if (request.method == "OPTIONS") {
      response.headers["access-control-allow-origin"] = "*";
      response.headers["access-control-allow-methods"] = "*";
      return request.end(response);
    }

    // handle 'data' requests from the dataManager
    if (message.name == "data") {
      if (!message.has("id")) {
        response.body.json = JSON::Object::Entries {
          {"source", "data"},
          {"err", JSON::Object::Entries {
            {"message", "Missing 'id' in message"}
          }}
        };
      } else {
        try {
          auto id = std::stoull(message.get("id"));
          if (!this->dataManager->has(id)) {
            response.body.json = JSON::Object::Entries {
              {"source", "data"},
              {"err", JSON::Object::Entries {
                {"type", "NotFoundError"},
                {"message", "No data associated with 'given 'id' in message"}
              }}
            };
          }

          auto data = this->dataManager->get(id);
          response.body.bytes = data.body;
          response.body.size = data.length;
          response.headers = data.headers;
          #if defined(__APPLE__)
            // 16ms timeout before removing data and potentially freeing `data.body`
            NSTimeInterval timeout = 0.16;
            auto block = ^(NSTimer* timer) {
              dispatch_async(dispatch_get_main_queue(), ^{
                this->dataManager->remove(id);
              });
            };

            [NSTimer timerWithTimeInterval: timeout repeats: NO block: block ];
          #endif
        } catch (...) {
          response.body.json = JSON::Object::Entries {
            {"source", "data"},
            {"err", JSON::Object::Entries {
              {"message", "Invalid 'id' given in message"}
            }}
          };
        }
      }

      return request.end(response);
    }

    if (message.seq.size() > 0 && message.seq != "-1") {
      #if defined(__APPLE__)
        auto task = (SchemeTask) request.internal;
        #if !__has_feature(objc_arc)
          [task retain];
        #endif

        auto schemeHandler = (::SchemeHandler*) this->internal;
        schemeHandler.taskManager->put(message.seq, task);
      #endif
    }

    if (this->onIPCSchemeRequestRouteCallback != nullptr) {
      if (this->onIPCSchemeRequestRouteCallback(request)) {
        return;
      }
    }

    response.statusCode = 404;
    response.body.json = JSON::Object::Entries {
      {"err", JSON::Object::Entries {
        {"message", "Not found"},
        {"type", "NotFoundError"},
        {"url", request.url}
      }}
    };

    request.end(response);
  }
}

#if defined(__APPLE__)
@implementation SchemeHandler
- (void) webView: (WKWebView*) webview stopURLSchemeTask: (SchemeTask) task {}
- (void) webView: (WKWebView*) webview startURLSchemeTask: (SchemeTask) task {
  auto request = ssc::internal::webview::SchemeRequest {
    .method = String(task.request.HTTPMethod.UTF8String),
    .url = String(task.request.URL.absoluteString.UTF8String)
  };

  auto body = task.request.HTTPBody;

  if (body) {
    request.body.bytes = (char*) [body bytes];
    request.body.size = [body length];
  }

  request.internal = (void *) task;
  #if !__has_feature(objc_arc)
    [task retain];
  #endif

  self.handler->onSchemeRequest(request);
}
@end
#endif
