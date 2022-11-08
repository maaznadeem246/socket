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

@implementation SSCNavigationDelegate
- (void) webview: (SSCBridgedWebView*) webview
    decidePolicyForNavigationAction: (WKNavigationAction*) navigationAction
    decisionHandler: (void (^)(WKNavigationActionPolicy)) decisionHandler {

  String base = webview.URL.absoluteString.UTF8String;
  String request = navigationAction.request.URL.absoluteString.UTF8String;

  if (request.find("file://") == 0 && request.find("http://localhost") == 0) {
    decisionHandler(WKNavigationActionPolicyCancel);
  } else {
    decisionHandler(WKNavigationActionPolicyAllow);
  }
}
@end

#if TARGET_OS_IPHONE || TARGET_IPHONE_SIMULATOR
@implementation SSCBridgedWebView
@end
#else
@interface SSCWindowDelegate : NSObject <NSWindowDelegate, WKScriptMessageHandler>
- (void) userContentController: (WKUserContentController*) userContentController
       didReceiveScriptMessage: (WKScriptMessage*) scriptMessage;
@end

@implementation SSCWindowDelegate
- (void) userContentController: (WKUserContentController*) userContentController
       didReceiveScriptMessage: (WKScriptMessage*) scriptMessage
{
  // overloaded with `class_replaceMethod()`
}
@end

@implementation SSCBridgedWebView
Vector<String> draggablePayload;

int lastX = 0;
int lastY = 0;

- (BOOL) prepareForDragOperation: (id<NSDraggingInfo>)info {
  [info setDraggingFormation: NSDraggingFormationNone];
  return NO;
}

- (void) draggingExited: (id<NSDraggingInfo>)info {
  NSPoint pos = [info draggingLocation];
  auto x = std::to_string(pos.x);
  auto y = std::to_string([self frame].size.height - pos.y);

  ssc::String json = (
    "{\"x\":" + x + ","
    "\"y\":" + y + "}"
  );

  auto payload = ssc::getEmitToRenderProcessJavaScript("dragend", json);
  draggablePayload.clear();

  [self evaluateJavaScript:
    [NSString stringWithUTF8String: payload.c_str()]
    completionHandler:nil];
}

- (NSDragOperation) draggingUpdated:(id <NSDraggingInfo>)info {
  NSPoint pos = [info draggingLocation];
  auto x = std::to_string(pos.x);
  auto y = std::to_string([self frame].size.height - pos.y);

  int count = draggablePayload.size();
  bool inbound = false;

  if (count == 0) {
    inbound = true;
    count = [info numberOfValidItemsForDrop];
  }

  ssc::String json = (
    "{\"count\":" + std::to_string(count) + ","
    "\"inbound\":" + (inbound ? "true" : "false") + ","
    "\"x\":" + x + ","
    "\"y\":" + y + "}"
  );

  auto payload = ssc::getEmitToRenderProcessJavaScript("drag", json);

  [self evaluateJavaScript:
    [NSString stringWithUTF8String: payload.c_str()] completionHandler:nil];
  return NSDragOperationGeneric;
}

- (NSDragOperation) draggingEntered: (id<NSDraggingInfo>)info {
  [self draggingUpdated: info];

  auto payload = ssc::getEmitToRenderProcessJavaScript("dragenter", "{}");
  [self evaluateJavaScript:
    [NSString stringWithUTF8String: payload.c_str()]
    completionHandler:nil];
  return NSDragOperationGeneric;
}

- (void) draggingEnded: (id<NSDraggingInfo>)info {
  NSPasteboard *pboard = [info draggingPasteboard];
  NSPoint pos = [info draggingLocation];
  int y = [self frame].size.height - pos.y;

  NSArray<Class> *classes = @[[NSURL class]];
  NSDictionary *options = @{};
  NSArray<NSURL*> *files = [pboard readObjectsForClasses:classes options:options];

  // if (NSPointInRect([info draggingLocation], self.frame)) {
    // NSWindow is (0,0) at bottom left, browser is (0,0) at top left
    // so we need to flip the y coordinate to convert to browser coordinates

  ssc::StringStream ss;
  int len = [files count];
  ss << "[";

  for (int i = 0; i < len; i++) {
    NSURL *url = files[i];
    ssc::String path = [[url path] UTF8String];
    // path = ssc::replace(path, "\"", "'");
    // path = ssc::replace(path, "\\", "\\\\");
    ss << "\"" << path << "\"";

    if (i < len - 1) {
      ss << ",";
    }
  }

  ss << "]";

  ssc::String json = (
    "{\"files\": " + ss.str() + ","
    "\"x\":" + std::to_string(pos.x) + ","
    "\"y\":" + std::to_string(y) + "}"
  );

  auto payload = ssc::getEmitToRenderProcessJavaScript("dropin", json);

  [self evaluateJavaScript:
    [NSString stringWithUTF8String: payload.c_str()] completionHandler:nil];
  // }

  // [super draggingEnded:info];
}

- (void) updateEvent: (NSEvent*)event {
  NSPoint location = [self convertPoint:[event locationInWindow] fromView:nil];
  auto x = std::to_string(location.x);
  auto y = std::to_string(location.y);
  auto count = std::to_string(draggablePayload.size());

  if (((int) location.x) == lastX || ((int) location.y) == lastY) {
    [super mouseDown:event];
    return;
  }

  ssc::String json = (
    "{\"count\":" + count + ","
    "\"x\":" + x + ","
    "\"y\":" + y + "}"
  );

  auto payload = ssc::getEmitToRenderProcessJavaScript("drag", json);

  [self evaluateJavaScript:
    [NSString stringWithUTF8String: payload.c_str()] completionHandler:nil];
}

- (void) mouseUp: (NSEvent*)event {
  [super mouseUp:event];

  NSPoint location = [self convertPoint:[event locationInWindow] fromView:nil];
  int x = location.x;
  int y = location.y;

  auto sx = std::to_string(x);
  auto sy = std::to_string(y);

  auto significantMoveX = (lastX - x) > 6 || (x - lastX) > 6;
  auto significantMoveY = (lastY - y) > 6 || (y - lastY) > 6;

  if (significantMoveX || significantMoveY) {
    for (auto path : draggablePayload) {
      path = ssc::replace(path, "\"", "'");

      ssc::String json = (
        "{\"src\":\"" + path + "\","
        "\"x\":" + sx + ","
        "\"y\":" + sy + "}"
      );

      auto payload = ssc::getEmitToRenderProcessJavaScript("drop", json);

      [self evaluateJavaScript:
        [NSString stringWithUTF8String: payload.c_str()]
        completionHandler:nil];
    }
  }

  ssc::String json = (
    "{\"x\":" + sx + ","
    "\"y\":" + sy + "}"
  );

  auto payload = ssc::getEmitToRenderProcessJavaScript("dragend", json);

  [self evaluateJavaScript:
    [NSString stringWithUTF8String: payload.c_str()] completionHandler:nil];
}

- (void) mouseDown: (NSEvent*)event {
  draggablePayload.clear();

  NSPoint location = [self convertPoint:[event locationInWindow] fromView:nil];
  auto x = std::to_string(location.x);
  auto y = std::to_string(location.y);

  lastX = (int) location.x;
  lastY = (int) location.y;

  ssc::String js(
    "(() => {"
    "  const el = document.elementFromPoint(" + x + "," + y + ");"
    "  if (!el) return;"
    "  const found = el.matches('[data-src]') ? el : el.closest('[data-src]');"
    "  return found && found.dataset.src"
    "})()");

  [self
    evaluateJavaScript: [NSString stringWithUTF8String:js.c_str()]
     completionHandler: ^(id result, NSError *error) {
    if (error) {
      NSLog(@"%@", error);
      [super mouseDown:event];
      return;
    }

    if (![result isKindOfClass:[NSString class]]) {
      [super mouseDown:event];
      return;
    }

    ssc::Vector<ssc::String> files =
      ssc::split(ssc::String([result UTF8String]), ';');

    if (files.size() == 0) {
      [super mouseDown:event];
      return;
    }

    draggablePayload = files;
    [self updateEvent: event];
  }];
}

- (void) mouseDragged: (NSEvent*)event {
  NSPoint location = [self convertPoint:[event locationInWindow] fromView:nil];
  [super mouseDragged:event];

  /* NSPoint currentLocation;
  NSPoint newOrigin;

  NSRect  screenFrame = [[NSScreen mainScreen] frame];
  NSRect  windowFrame = [self frame];

  currentLocation = [NSEvent mouseLocation];
  newOrigin.x = currentLocation.x - lastX;
  newOrigin.y = currentLocation.y - lastY;

  if ((newOrigin.y+windowFrame.size.height) > (screenFrame.origin.y+screenFrame.size.height)){
    newOrigin.y=screenFrame.origin.y + (screenFrame.size.height-windowFrame.size.height);
  }

  [[self window] setFrameOrigin:newOrigin]; */

  if (!NSPointInRect(location, self.frame)) {
    auto payload = ssc::getEmitToRenderProcessJavaScript("dragexit", "{}");
    [self evaluateJavaScript:
      [NSString stringWithUTF8String: payload.c_str()] completionHandler:nil];
  }

  if (draggablePayload.size() == 0) return;

  int x = location.x;
  int y = location.y;

  auto significantMoveX = (lastX - x) > 6 || (x - lastX) > 6;
  auto significantMoveY = (lastY - y) > 6 || (y - lastY) > 6;

  if (significantMoveX || significantMoveY) {
    auto sx = std::to_string(x);
    auto sy = std::to_string(y);
    auto count = std::to_string(draggablePayload.size());

    ssc::String json = (
      "{\"count\":" + count + ","
      "\"x\":" + sx + ","
      "\"y\":" + sy + "}"
    );

    auto payload = ssc::getEmitToRenderProcessJavaScript("drag", json);

    [self evaluateJavaScript:
      [NSString stringWithUTF8String: payload.c_str()] completionHandler:nil];
  }

  if (NSPointInRect(location, self.frame)) return;

  NSPasteboard *pboard = [NSPasteboard pasteboardWithName: NSPasteboardNameDrag];
  [pboard declareTypes: @[(NSString*) kPasteboardTypeFileURLPromise] owner:self];

  NSMutableArray* dragItems = [[NSMutableArray alloc] init];
  NSSize iconSize = NSMakeSize(32, 32); // according to documentation
  NSRect imageLocation;
  NSPoint dragPosition = [self
    convertPoint: [event locationInWindow]
    fromView: nil];

  dragPosition.x -= 16;
  dragPosition.y -= 16;
  imageLocation.origin = dragPosition;
  imageLocation.size = iconSize;

  for (auto& file : draggablePayload) {
    NSString* nsFile = [[NSString alloc] initWithUTF8String:file.c_str()];
    NSURL* fileURL = [NSURL fileURLWithPath: nsFile];

    NSImage* icon = [[NSWorkspace sharedWorkspace] iconForContentType: UTTypeURL];

    NSArray* (^providerBlock)() = ^NSArray*() {
      NSDraggingImageComponent* comp = [[[NSDraggingImageComponent alloc]
        initWithKey: NSDraggingImageComponentIconKey] retain];

      comp.frame = NSMakeRect(0, 0, iconSize.width, iconSize.height);
      comp.contents = icon;
      return @[comp];
    };

    NSDraggingItem* dragItem;
    auto provider = [[NSFilePromiseProvider alloc] initWithFileType:@"public.url" delegate: self];
    [provider setUserInfo: [NSString stringWithUTF8String:file.c_str()]];
    dragItem = [[NSDraggingItem alloc] initWithPasteboardWriter: provider];

    dragItem.draggingFrame = NSMakeRect(
      dragPosition.x,
      dragPosition.y,
      iconSize.width,
      iconSize.height
    );

    dragItem.imageComponentsProvider = providerBlock;
    [dragItems addObject: dragItem];
  }

  NSDraggingSession* session = [self
      beginDraggingSessionWithItems: dragItems
                              event: event
                             source: self];

  session.draggingFormation = NSDraggingFormationPile;
  draggablePayload.clear();
}

- (NSDragOperation)draggingSession:(NSDraggingSession*)session
    sourceOperationMaskForDraggingContext:(NSDraggingContext)context {
  return NSDragOperationCopy;
}

- (void) filePromiseProvider:(NSFilePromiseProvider*)filePromiseProvider writePromiseToURL:(NSURL *)url
  completionHandler:(void (^)(NSError *errorOrNil))completionHandler
{
  ssc::String dest = [[url path] UTF8String];
  ssc::String src([[filePromiseProvider userInfo] UTF8String]);

  NSData *data = [@"" dataUsingEncoding:NSUTF8StringEncoding];
  [data writeToURL:url atomically:YES];

  ssc::String json = (
    "{\"src\":\"" + src + "\","
    "\"dest\":\"" + dest + "\"}"
  );

  ssc::String js = ssc::getEmitToRenderProcessJavaScript("dropout", json);

  [self
    evaluateJavaScript: [NSString stringWithUTF8String:js.c_str()]
     completionHandler: nil
  ];

  completionHandler(nil);
}

- (NSString*) filePromiseProvider: (NSFilePromiseProvider*)filePromiseProvider fileNameForType:(NSString *)fileType {
  ssc::String file(std::to_string(ssc::rand64()) + ".download");
  return [NSString stringWithUTF8String:file.c_str()];
}
@end
#endif
