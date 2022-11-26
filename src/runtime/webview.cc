#include <socket/utils.hh>
#include "webview.hh"
#include "window.hh"

using namespace ssc;
using namespace ssc::runtime;
using namespace ssc::runtime::window;
using namespace ssc::runtime::webview;

#if defined(__APPLE__)
auto constexpr DISPATCH_QUEUE_LABEL = "co.socketsupply.queue.webview";
dispatch_queue_attr_t DISPATCH_QUEUE_QOS = dispatch_queue_attr_make_with_qos_class(
  DISPATCH_QUEUE_CONCURRENT,
  QOS_CLASS_DEFAULT,
  -1
);

dispatch_queue_t dispatchQueue = dispatch_queue_create(
  DISPATCH_QUEUE_LABEL,
  DISPATCH_QUEUE_QOS
);
#endif

/**
 *  implementations for Linux.
 */
#if defined(__linux__) && !defined(__ANDROID__)
/* TODO
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
*/
class LinuxSchemeTask {
};
using SchemeTask = SharedPointer<LinuxSchemeTask>
#endif

namespace ssc::runtime::webview {
  WebViewInternals::WebViewInternals (
    WebView* webview,
    Window* window,
    DataManager* dataManager,
    SchemeHandler* ipcSchemeHandler,
    const Script preloadScript
  ) {
    this->webview = webview;
    this->window = window;

  #if defined(__APPLE__)
    auto configuration = [WKWebViewConfiguration new];
    auto preferences = [configuration preferences];
    auto userScript = [WKUserScript alloc];
    // https://webkit.org/blog/10882/app-bound-domains/
    // https://developer.apple.com/documentation/webkit/wkwebviewconfiguration/3585117-limitsnavigationstoappbounddomai
    // configuration.limitsNavigationsToAppBoundDomains = YES;

    [configuration
      setURLSchemeHandler: (__bridge ::CoreSchemeHandler*) ipcSchemeHandler->internal
             forURLScheme: @"ipc"
    ];

    [preferences
      setJavaScriptCanOpenWindowsAutomatically: NO
    ];

    // Adds "Inspect" option to context menus if debug is enable
    [preferences
      setValue: ssc::config::isDebugEnabled() ? @YES : @NO
        forKey: @"developerExtrasEnabled"
    ];

    #if TARGET_OS_IPHONE || TARGET_IPHONE_SIMULATOR
      [preferences
        setValue: @YES
          forKey: @"javaScriptEnabled"
      ];
    #endif

    (void) [userScript
        initWithSource: [NSString stringWithUTF8String: preloadScript.str().c_str()]
         injectionTime: WKUserScriptInjectionTimeAtDocumentStart
      forMainFrameOnly: NO
    ];

    [configuration.userContentController
      addUserScript: userScript
    ];

    // Set delegate to window
    [configuration.userContentController
      addScriptMessageHandler: window->internals->coreDelegate
                         name: @"external"
    ];

    this->coreNavigationDelegate = [[CoreNavigationDelegate alloc] init];
    #if TARGET_OS_IPHONE || TARGET_IPHONE_SIMULATOR
      auto frame = [[UIScreen mainScreen] bounds];
      this->coreWebView = [[::CoreWKWebView alloc]
        initWithFrame: frame
        configuration: configuration
      ];
    #else
      this->coreWebView = [[::CoreWKWebView alloc]
        initWithFrame: NSZeroRect
        configuration: configuration
      ];
    #endif

    [this->coreWebView
      setNavigationDelegate: this->coreNavigationDelegate
    ];

    [this->coreWebView.configuration.preferences
      setValue: @YES
        forKey: @"allowFileAccessFromFileURLs"
    ];

    #if TARGET_OS_IPHONE || TARGET_IPHONE_SIMULATOR
      this->coreWebView.autoresizingMask = (
        UIViewAutoresizingFlexibleWidth |
        UIViewAutoresizingFlexibleHeight
      );
    #endif

    /* FIXME
    [this->webview
      setValue: [NSNumber numberWithBool: YES]
        forKey: @"drawsTransparentBackground"
    ];
    */

    /* FIXME
    [this->coreWebView
      registerForDraggedTypes: [NSArray arrayWithObject:NSPasteboardTypeFileURL]
    ];
    */

    /* FIXME
    [[NSNotificationCenter defaultCenter]
       addObserver: this->coreWebView
          selector: @selector(systemColorsDidChangeNotification:)
              name: NSSystemColorsDidChangeNotification
            object: nil
    ];
    */
  #endif
  }

  WebViewInternals::~WebViewInternals () {
  #if defined(__APPLE__)
    if (this->coreWebView != nullptr) {
      #if !__has_feature(objc_arc)
        [this->coreWebView release];
      #endif
      this->coreWebView = nullptr;
    }
  #endif
  }

  WebView::WebView (
    Window* window,
    DataManager* dataManager,
    IPCSchemeRequestRouteCallback onIPCSchemeRequestRouteCallback,
    const Script preloadScript
  ) {
    this->ipcSchemeHandler = new IPCSchemeHandler(
      dataManager,
      onIPCSchemeRequestRouteCallback
    );
    this->dataManager = dataManager;
    this->window = window;
    this->internals = new WebViewInternals(
      this,
      window,
      dataManager,
      this->ipcSchemeHandler,
      preloadScript
    );
  }

  WebView::~WebView () {
    if (this->internals != nullptr) {
      delete this->internals;
    }

    if (this->ipcSchemeHandler != nullptr) {
      delete this->ipcSchemeHandler;
    }
  }

  SchemeHandler::SchemeHandler (
    const String& scheme,
    DataManager* dataManager,
    SchemeRequestCallback onSchemeRequestCallback
  ) {
    this->scheme = scheme;
    this->dataManager = dataManager;
    this->onSchemeRequestCallback = onSchemeRequestCallback;

    #if defined(__APPLE__)
      auto schemeHandler = [::CoreSchemeHandler new];
      schemeHandler.handler = this;
      schemeHandler.taskManager = new SchemeTaskManager<CoreSchemeTask>();
      this->internal = (__bridge void*) schemeHandler;
    #elif defined(__linux__) && !defined(__ANDROID__)
      auto schemeHandler = SharedPointer<SchemeHandler>(new SchemeHandler());
      schemeHandler->handler = this;
      schemeHandler->taskManager = SharedPointer<SchemeTaskManager<CoreSchemeTask>>(new SchemeTaskManager<CoreSchemeTask>());
    #endif
  }

  SchemeHandler::~SchemeHandler () {
    #if defined(__APPLE__)
      if (this->internal != nullptr) {
        auto schemeHandler = (__bridge ::CoreSchemeHandler*) this->internal;
        delete schemeHandler.taskManager;
        #if !__has_feature(objc_arc)
          [schemeHandler release];
        #endif
        this->internal = nullptr;
      }
    #endif
  }

  void SchemeRequest::end (
    const SchemeResponseStatusCode statusCode,
    const SchemeResponseHeaders headers,
    const JSON::Any json
  ) {
    this->end(statusCode, headers, json, nullptr, 0);
  }

  void SchemeRequest::end (
    const SchemeResponseStatusCode statusCode,
    const SchemeResponseHeaders headers,
    const SchemeResponseBody body
  ) {
    this->end(statusCode, headers, body.json, body.bytes, body.size);
  }

  void SchemeRequest::end (
    const SchemeResponseStatusCode statusCode,
    const SchemeResponseHeaders headers,
    const char* bytes,
    size_t size
  ) {
    this->end(statusCode, headers, nullptr, bytes, size);
  }

  void SchemeRequest::end (
    const SchemeResponseStatusCode statusCode,
    const SchemeResponseHeaders headers,
    const JSON::Any json,
    const char* bytes,
    size_t size
  ) {
    this->response.statusCode = statusCode;
    this->response.headers = headers;
    this->response.body.json = json;
    this->response.body.size = size;
    this->response.body.bytes = (char*) bytes;

    #if defined(__APPLE__)
      if (this->internal == nullptr) {
        return;
      }

      auto jsonString = this->response.body.json.str();
      auto task = (__bridge CoreSchemeTask) this->internal;
      auto headerFields = [NSMutableDictionary dictionary];

      headerFields[@"access-control-allow-methods"] = @"*";
      headerFields[@"access-control-allow-origin"] = @"*";

      if (this->response.body.size > 0) {
        headerFields[@"content-length"] = [@(this->response.body.size) stringValue];
        headerFields[@"content-type"] = @"application/octet-stream";
      } else {
        headerFields[@"content-length"] = [@(jsonString.size()) stringValue];
        headerFields[@"content-type"] = @"application/json";
      }

      for (const auto& header : this->response.headers.entries) {
        auto key = [NSString stringWithUTF8String: trim(header.key).c_str()];
        auto value = [NSString stringWithUTF8String: trim(header.value.str()).c_str()];
        headerFields[key] = value;
      }

      auto taskResponse = [[NSHTTPURLResponse alloc]
         initWithURL: task.request.URL
          statusCode: response.statusCode
         HTTPVersion: @"HTTP/1.1"
        headerFields: headerFields
      ];

      [task didReceiveResponse: taskResponse];

      if (this->response.body.bytes) {
        auto data = [NSData
          dataWithBytes: this->response.body.bytes
                 length: this->response.body.size
        ];

        [task didReceiveData: data];
      } else {
        auto string = [NSString stringWithUTF8String: jsonString.c_str()];
        auto data = [string dataUsingEncoding: NSUTF8StringEncoding];
        [task didReceiveData: data];
      }

      [task didFinish];
      #if !__has_feature(objc_arc)
        [taskResponse release];
        [task release];
      #endif
    #endif

    this->internal = nullptr;
  }

  IPCSchemeHandler::IPCSchemeHandler (
    DataManager* dataManager,
    IPCSchemeRequestRouteCallback onIPCSchemeRequestRouteCallback
  ) : SchemeHandler("ipc", dataManager, nullptr) {
    this->onIPCSchemeRequestRouteCallback = onIPCSchemeRequestRouteCallback;
  }

  void IPCSchemeHandler::onSchemeRequest (SchemeRequest& request) {
    auto message = Message(request.url);
    SchemeResponseStatusCode statusCode = 200;
    SchemeResponseHeaders headers;
    SchemeResponseBody body;

    if (request.method == "OPTIONS") {
      return request.end(statusCode, headers, body);
    }

    // validate 'data' requests
    if (message.name == "data") {
      if (!message.has("id")) {
        statusCode = 400;
        body.json = JSON::Object::Entries {
          {"source", "data"},
          {"err", JSON::Object::Entries {
            {"message", "Missing 'id' in message"}
          }}
        };

        return request.end(statusCode, headers, body);
      } else {
        try {
          auto id = std::stoull(message.get("id", "0"));
          if (!this->dataManager->has(id)) {
            statusCode = 404;
            body.json = JSON::Object::Entries {
              {"source", "data"},
              {"err", JSON::Object::Entries {
                {"type", "NotFoundError"},
                {"message", "No data associated with 'given 'id' in message"}
              }}
            };
          }

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
          statusCode = 400;
          body.json = JSON::Object::Entries {
            {"source", "data"},
            {"err", JSON::Object::Entries {
              {"message", "Invalid 'id' given in message"}
            }}
          };

          return request.end(statusCode, headers, body);
        }
      }
    }

    if (message.seq.size() > 0 && message.seq != "-1") {
      #if defined(__APPLE__)
        auto task = (__bridge CoreSchemeTask) request.internal;
        #if !__has_feature(objc_arc)
          [task retain];
        #endif

        auto schemeHandler = (__bridge ::CoreSchemeHandler*) this->internal;
        schemeHandler.taskManager->put(message.seq, task);
      #endif
    }

    if (this->onIPCSchemeRequestRouteCallback != nullptr) {
      if (this->onIPCSchemeRequestRouteCallback(request)) {
        return;
      }
    }

    statusCode = 404;
    body.json = JSON::Object::Entries {
      {"source", message.name},
      {"err", JSON::Object::Entries {
        {"message", "Not found"},
        {"type", "NotFoundError"},
        {"url", request.url}
      }}
    };

    request.end(statusCode, headers, body);
  }
}

#if defined(__APPLE__)
@implementation CoreSchemeHandler
- (void) webView: (CoreWKWebView*) webview stopURLSchemeTask: (CoreSchemeTask) task {}
- (void) webView: (CoreWKWebView*) webview startURLSchemeTask: (CoreSchemeTask) task {
  auto request = ssc::runtime::webview::SchemeRequest {
    String(task.request.HTTPMethod.UTF8String),
    String(task.request.URL.absoluteString.UTF8String),
    { nullptr, 0 }
  };

  auto body = task.request.HTTPBody;

  if (body) {
    request.body.bytes = (char*) [body bytes];
    request.body.size = [body length];
    request.message.buffer.bytes = request.body.bytes;
    request.message.buffer.size = request.body.size;
  }

  request.internal = (__bridge void *) task;
  #if !__has_feature(objc_arc)
    [task retain];
  #endif

  dispatch_async(dispatchQueue, ^{
    ssc::runtime::webview::SchemeRequest req(request);
    self.handler->onSchemeRequest(req);
  });
}
@end

@implementation CoreNavigationDelegate
- (void) webview: (CoreWKWebView*) webview
    decidePolicyForNavigationAction: (WKNavigationAction*) navigationAction
                    decisionHandler: (void (^)(WKNavigationActionPolicy)) decisionHandler {
  String base = webview.URL.absoluteString.UTF8String;
  String url = navigationAction.request.URL.absoluteString.UTF8String;

  if (url.find("file://") == 0 && url.find("http://localhost") == 0) {
    decisionHandler(WKNavigationActionPolicyCancel);
  } else {
    decisionHandler(WKNavigationActionPolicyAllow);
  }
}
@end

@implementation CoreWKWebView

#if !TARGET_OS_IPHONE && !TARGET_OS_IPHONE
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

  String json = (
    "{\"x\":" + x + ","
    "\"y\":" + y + "}"
  );

  auto payload = getEmitToRenderProcessJavaScript("dragend", json);
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

  String json = (
    "{\"count\":" + std::to_string(count) + ","
    "\"inbound\":" + (inbound ? "true" : "false") + ","
    "\"x\":" + x + ","
    "\"y\":" + y + "}"
  );

  auto payload = getEmitToRenderProcessJavaScript("drag", json);

  [self evaluateJavaScript:
    [NSString stringWithUTF8String: payload.c_str()] completionHandler:nil];
  return NSDragOperationGeneric;
}

- (NSDragOperation) draggingEntered: (id<NSDraggingInfo>)info {
  [self draggingUpdated: info];

  auto payload = getEmitToRenderProcessJavaScript("dragenter", "{}");
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

  StringStream ss;
  int len = [files count];
  ss << "[";

  for (int i = 0; i < len; i++) {
    NSURL *url = files[i];
    String path = [[url path] UTF8String];
    // path = ssc::replace(path, "\"", "'");
    // path = ssc::replace(path, "\\", "\\\\");
    ss << "\"" << path << "\"";

    if (i < len - 1) {
      ss << ",";
    }
  }

  ss << "]";

  String json = (
    "{\"files\": " + ss.str() + ","
    "\"x\":" + std::to_string(pos.x) + ","
    "\"y\":" + std::to_string(y) + "}"
  );

  auto payload = getEmitToRenderProcessJavaScript("dropin", json);

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

  String json = (
    "{\"count\":" + count + ","
    "\"x\":" + x + ","
    "\"y\":" + y + "}"
  );

  auto payload = getEmitToRenderProcessJavaScript("drag", json);

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
      path = replace(path, "\"", "'");

      String json = (
        "{\"src\":\"" + path + "\","
        "\"x\":" + sx + ","
        "\"y\":" + sy + "}"
      );

      auto payload = getEmitToRenderProcessJavaScript("drop", json);

      [self evaluateJavaScript:
        [NSString stringWithUTF8String: payload.c_str()]
        completionHandler:nil];
    }
  }

  String json = (
    "{\"x\":" + sx + ","
    "\"y\":" + sy + "}"
  );

  auto payload = getEmitToRenderProcessJavaScript("dragend", json);

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

  String js(
    "(() => {"
    "  const el = document.elementFromPoint(" + x + "," + y + ");"
    "  if (!el) return;"
    "  const found = el.matches('[data-src]') ? el : el.closest('[data-src]');"
    "  return found && found.dataset.src"
    "})()"
  );

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

    Vector<String> files =
      split(String([result UTF8String]), ';');

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
    auto payload = getEmitToRenderProcessJavaScript("dragexit", "{}");
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

    String json = (
      "{\"count\":" + count + ","
      "\"x\":" + sx + ","
      "\"y\":" + sy + "}"
    );

    auto payload = getEmitToRenderProcessJavaScript("drag", json);

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
  String dest = [[url path] UTF8String];
  String src([[filePromiseProvider userInfo] UTF8String]);

  NSData *data = [@"" dataUsingEncoding:NSUTF8StringEncoding];
  [data writeToURL:url atomically:YES];

  String json = (
    "{\"src\":\"" + src + "\","
    "\"dest\":\"" + dest + "\"}"
  );

  String js = getEmitToRenderProcessJavaScript("dropout", json);

  [self
    evaluateJavaScript: [NSString stringWithUTF8String:js.c_str()]
     completionHandler: nil
  ];

  completionHandler(nil);
}

- (NSString*) filePromiseProvider: (NSFilePromiseProvider*)filePromiseProvider fileNameForType:(NSString *)fileType {
  String file(std::to_string(utils::rand64()) + ".download");
  return [NSString stringWithUTF8String:file.c_str()];
}
#endif // !TARGET_OS_IPHONE && !TARGET_IPHONE_SIMULATOR
@end

#endif // __APPLE__
