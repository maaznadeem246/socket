#include "ipc/message.hh"
#include "json.hh"
#include "string.hh"
#include "types.hh"
#include "webview.hh"

#if defined(__APPLE__)
#import <WebKit/WebKit.h>
#endif

using namespace ssc::types;
using ssc::string::split;
using ssc::string::trim;

#if defined(__APPLE__)
using Task = id<WKURLSchemeTask>;
@interface SchemeHandler : NSObject<WKURLSchemeHandler>
@property (nonatomic) ssc::webview::SchemeHandler* schemeHandler;
- (void) webView: (WKWebView*) webview startURLSchemeTask: (Task) task;
- (void) webView: (WKWebView*) webview stopURLSchemeTask: (Task) task;
@end
#endif

namespace ssc::webview {
  SchemeHandler::SchemeHandler (const String& scheme) {
    this->scheme = scheme;
  }

  SchemeHandler::SchemeHandler (
    const String& scheme,
    SchemeRequestCallback onSchemeRequestCallback
  ) {
    this->scheme = scheme;
    this->onSchemeRequestCallback = onSchemeRequestCallback;
    #if defined(__APPLE__)
      auto schemeHandler = [::SchemeHandler new];
      schemeHandler.schemeHandler = this;
      this->internal = (void*) schemeHandler;
    #endif
  }

  SchemeHandler::~SchemeHandler () {
    #if defined(__APPLE__)
      if (this->internal != nullptr) {
        auto schemeHandler = (::SchemeHandler*) this->internal;
        this->internal = nullptr;
        [schemeHandler release];
      }
    #endif
  }

  void SchemeHandler::onSchemeRequest (SchemeRequest& request) {
    if (this->onSchemeRequestCallback != nullptr) {
      this->onSchemeRequestCallback(request);
    }
  }

  void SchemeRequest::end (SchemeResponse& response) {
    this->end(
      response.statusCode,
      response.headers,
      response.body.bytes,
      response.body.size
    );
  }

  void SchemeRequest::end (int statusCode, Map headers, char *bytes, size_t size) {
    #if defined(__APPLE__)
      auto task = (Task) this->internal;
      auto responseUrl = [NSURL URLWithString: [NSString stringWithUTF8String: this->url.c_str()]];
      auto responseHeaders = [NSMutableDictionary dictionary];

      for (const auto& header : headers) {
        auto key = [NSString stringWithUTF8String: trim(header.first).c_str()];
        auto value = [NSString stringWithUTF8String: trim(header.second).c_str()];
        responseHeaders[key] = value;
      }

      auto response = [[NSHTTPURLResponse alloc]
         initWithURL: responseUrl
          statusCode: statusCode
         HTTPVersion: @"HTTP/1.1"
        headerFields: responseHeaders
      ];

      [task didReceiveResponse: response];

      if (bytes) {
        auto data = [NSData dataWithBytes: bytes length: size];
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
    #endif
  }

  IPCSchemeHandler::IPCSchemeHandler () : SchemeHandler("ipc") {
    this->onSchemeRequestCallback = [this](auto& request) {
      auto response = SchemeResponse { &request };

      if (request.method == "OPTIONS") {
        response.headers["access-control-allow-origin"] = "*";
        response.headers["access-control-allow-methods"] = "*";
      }

      request.end(response);
    };
  }
}

#if defined(__APPLE__)
@implementation SchemeHandler
- (void) webView: (WKWebView*) webview stopURLSchemeTask: (Task) task {}
- (void) webView: (WKWebView*) webview startURLSchemeTask: (Task) task {
  auto request = ssc::webview::SchemeRequest {
    String(task.request.HTTPMethod.UTF8String),
    String(task.request.URL.absoluteString.UTF8String)
  };

  auto body = task.request.HTTPBody;

  if (body) {
    request.body.bytes = (char*) [body bytes];
    request.body.size = [body length];
  }

  request.internal = task;
  #if !__has_feature(objc_arc)
    [task retain];
  #endif

  self.schemeHandler->onSchemeRequest(request);

  /*
  if (message.name == "post") {
    auto headers = [NSMutableDictionary dictionary];
    auto id = std::stoull(message.get("id"));
    auto post = self.router->core->getPost(id);

    headers[@"access-control-allow-origin"] = @"*";
    headers[@"content-length"] = [@(post.length) stringValue];

    if (post.headers.size() > 0) {
      auto lines = split(trim(post.headers), '\n');

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
  */
}
@end
#endif
