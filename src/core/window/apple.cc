#include "../private.hh"
#include "../window.hh"

#if defined(__APPLE__)

#if TARGET_OS_IPHONE || TARGET_IPHONE_SIMULATOR
@interface CoreWindowDelegate : NSObject
@end
#else
@interface CoreWindowDelegate : NSObject <NSWindowDelegate, WKScriptMessageHandler>
- (void) userContentController: (WKUserContentController*) userContentController
       didReceiveScriptMessage: (WKScriptMessage*) scriptMessage;
@end

@implementation CoreWindowDelegate
- (void) userContentController: (WKUserContentController*) userContentController
       didReceiveScriptMessage: (WKScriptMessage*) scriptMessage
{
  // overloaded with `class_replaceMethod()`
}
@end
#endif

namespace ssc::core::window {
  using namespace application;
  using namespace string;

  static bool isDelegateSet = false;

  class CoreWindowInternals {
    public:
    #if !TARGET_OS_IPHONE && !TARGET_IPHONE_SIMULATOR
      NSWindow* window;
    #endif

      CoreWindowInternals (const CoreWindowOptions& opts);
  };

  CoreWindowInternals::CoreWindowInternals (const CoreWindowOptions& opts) {
    // Window style: titled, closable, minimizable
    uint style = NSWindowStyleMaskTitled;

    // Set window to be resizable
    if (opts.resizable) {
      style |= NSWindowStyleMaskResizable;
    }

    if (opts.frameless) {
      style |= NSWindowStyleMaskFullSizeContentView;
      style |= NSWindowStyleMaskBorderless;
    } else if (opts.utility) {
      style |= NSWindowStyleMaskUtilityWindow;
    } else {
      style |= NSWindowStyleMaskClosable;
      style |= NSWindowStyleMaskMiniaturizable;
    }

    auto draggableTypes = [NSArray arrayWithObjects:
      NSPasteboardTypeURL,
      NSPasteboardTypeFileURL,
      (NSString*) kPasteboardTypeFileURLPromise,
      NSPasteboardTypeString,
      NSPasteboardTypeHTML,
      nil
		];

    this->window = [[NSWindow alloc]
        initWithContentRect: NSMakeRect(0, 0, opts.width, opts.height)
                  styleMask: style
                    backing: NSBackingStoreBuffered
                      defer: NO];

    // Position window in center of screen
    [window center];
    [window setOpaque: YES];
    // Minimum window size
    [window setContentMinSize: NSMakeSize(opts.width, opts.height)];
    [window setBackgroundColor: [NSColor controlBackgroundColor]];
    [window registerForDraggedTypes: draggableTypes];
    // [window setAppearance:[NSAppearance appearanceNamed:NSAppearanceNameVibrantDark]];
    // [window setMovableByWindowBackground: true];

    if (opts.frameless) {
      [window setTitlebarAppearsTransparent: true];
    }
  }

  CoreWindow::CoreWindow (
    CoreApplication& app,
    const CoreWindowOptions opts
  ) : CoreWindow(app) {
    this->opts = opts;
    this->internals = new CoreWindowInternals(opts);


    this->bridge = new IPC::Bridge(app.runtime);
    this->bridge->router.dispatchFunction = [this] (auto callback) {
      this->app.dispatch(callback);
    };
    this->bridge->router.evaluateJavaScriptFunction = [this](auto js) {
      dispatch_async(dispatch_get_main_queue(), ^{ this->eval(js); });
    };

    this->bridge->router.map("window.eval", [=](auto message, auto router, auto reply) {
      auto value = message.value;
      auto seq = message.seq;
      auto  script = [NSString stringWithUTF8String: value.c_str()];

      dispatch_async(dispatch_get_main_queue(), ^{
        [this->internals->webview evaluateJavaScript: script completionHandler: ^(id result, NSError *error) {
          if (result) {
            auto msg = String([[NSString stringWithFormat:@"%@", result] UTF8String]);
            this->bridge->router.send(seq, msg, Post{});
          } else if (error) {
            auto exception = (NSString *) error.userInfo[@"WKJavaScriptExceptionMessage"];
            auto message = [[NSString stringWithFormat:@"%@", exception] UTF8String];
            auto err = encodeURIComponent(String(message));

            if (err == "(null)") {
              this->bridge->router.send(seq, "null", Post{});
              return;
            }

            auto json = JSON::Object::Entries {
              {"err", JSON::Object::Entries {
                {"message", String("Error: ") + err}
              }}
            };

            this->bridge->router.send(seq, JSON::Object(json).str(), Post{});
          } else {
            this->bridge->router.send(seq, "undefined", Post{});
          }
        }];
      });
    });


    // Set window to be resizable
    if (opts.resizable) {
      style |= NSWindowStyleMaskResizable;
    }

    if (opts.frameless) {
      style |= NSWindowStyleMaskFullSizeContentView;
      style |= NSWindowStyleMaskBorderless;
    } else if (opts.utility) {
      style |= NSWindowStyleMaskUtilityWindow;
    } else {
      style |= NSWindowStyleMaskClosable;
      style |= NSWindowStyleMaskMiniaturizable;
    }

    ScreenSize screenSize = this->getScreenSize();
    auto height = opts.isHeightInPercent ? screenSize.height * opts.height / 100 : opts.height;
    auto width = opts.isWidthInPercent ? screenSize.width * opts.width / 100 : opts.width;

    window = [[NSWindow alloc]
        initWithContentRect: NSMakeRect(0, 0, width, height)
                  styleMask: style
                    backing: NSBackingStoreBuffered
                      defer: NO];

    NSArray* draggableTypes = [NSArray arrayWithObjects:
      NSPasteboardTypeURL,
      NSPasteboardTypeFileURL,
      (NSString*) kPasteboardTypeFileURLPromise,
      NSPasteboardTypeString,
      NSPasteboardTypeHTML,
      nil
		];

    // Position window in center of screen
    [window center];
    [window setOpaque: YES];
    // Minimum window size
    [window setContentMinSize: NSMakeSize(opts.width, opts.height)];
    [window setBackgroundColor: [NSColor controlBackgroundColor]];
    [window registerForDraggedTypes: draggableTypes];
    // [window setAppearance:[NSAppearance appearanceNamed:NSAppearanceNameVibrantDark]];

    if (opts.frameless) {
      [window setTitlebarAppearsTransparent: true];
    }

    // window.movableByWindowBackground = true;
    window.titlebarAppearsTransparent = true;

    // Initialize WKWebView
    WKWebViewConfiguration* config = [WKWebViewConfiguration new];
    // https://webkit.org/blog/10882/app-bound-domains/
    // https://developer.apple.com/documentation/webkit/wkwebviewconfiguration/3585117-limitsnavigationstoappbounddomai
    // config.limitsNavigationsToAppBoundDomains = YES;

    [config setURLSchemeHandler: bridge->router.schemeHandler
                   forURLScheme: @"ipc"];

    [config setURLSchemeHandler: bridge->router.schemeHandler
                   forURLScheme: @"ssc"];

    WKPreferences* prefs = [config preferences];
    [prefs setJavaScriptCanOpenWindowsAutomatically:NO];

    #if DEBUG == 1 // Adds "Inspect" option to context menus
      [prefs setValue:@YES forKey:@"developerExtrasEnabled"];
    #endif

    WKUserContentController* controller = [config userContentController];

    // Add preload script, normalizing the interface to be cross-platform.
    String preload = ToString(createPreload(opts));

    WKUserScript* userScript = [WKUserScript alloc];

    [userScript
      initWithSource: [NSString stringWithUTF8String:preload.c_str()]
      injectionTime: WKUserScriptInjectionTimeAtDocumentStart
      forMainFrameOnly: NO];

    [controller addUserScript: userScript];

    webview = [[SSCBridgedWebView alloc]
      initWithFrame: NSZeroRect
      configuration: config];

    [webview.configuration.preferences
      setValue:@YES
      forKey:@"allowFileAccessFromFileURLs"];

    /* [webview
      setValue: [NSNumber numberWithBool: YES]
        forKey: @"drawsTransparentBackground"
    ]; */

    /* [[NSNotificationCenter defaultCenter] addObserver: webview
                                             selector: @selector(systemColorsDidChangeNotification:)
                                                 name: NSSystemColorsDidChangeNotification
                                               object: nil
    ]; */

    // [webview registerForDraggedTypes:
    //  [NSArray arrayWithObject:NSPasteboardTypeFileURL]];
    //
    bool exiting = false;

    SSCWindowDelegate* delegate = [SSCWindowDelegate alloc];
    [controller addScriptMessageHandler: delegate name: @"external"];

    // Set delegate to window
    [window setDelegate:delegate];

    SSCNavigationDelegate *navDelegate = [[SSCNavigationDelegate alloc] init];
    [webview setNavigationDelegate: navDelegate];

    if (!isDelegateSet) {
      isDelegateSet = true;

      class_replaceMethod(
        [SSCWindowDelegate class],
        @selector(windowShouldClose:),
        imp_implementationWithBlock(
          [&](id self, SEL cmd, id notification) {
            if (exiting) return true;

            auto* w = (Window*) objc_getAssociatedObject(self, "webview");

            if (w->opts.canExit) {
              exiting = true;
              w->exit(0);
              return true;
            }

            w->eval(getEmitToRenderProcessJavaScript("windowHide", "{}"));
            w->hide("");
            return false;
          }),
        "v@:@"
      );

      class_replaceMethod(
        [SSCWindowDelegate class],
        @selector(userContentController:didReceiveScriptMessage:),
        imp_implementationWithBlock(
          [=](id self, SEL cmd, WKScriptMessage* scriptMessage) {
            auto* w = (Window*) objc_getAssociatedObject(self, "webview");
            if (w->onMessage == nullptr) return;

            id body = [scriptMessage body];
            if (![body isKindOfClass:[NSString class]]) {
              return;
            }
            String msg = [body UTF8String];

            if (bridge->route(msg, nullptr, 0)) return;
            w->onMessage(msg);
          }),
        "v@:@"
      );

      class_addMethod(
        [SSCWindowDelegate class],
        @selector(menuItemSelected:),
        imp_implementationWithBlock(
          [=](id self, SEL _cmd, id item) {
            auto* w = (Window*) objc_getAssociatedObject(self, "webview");
            if (w->onMessage == nullptr) return;

            id menuItem = (id) item;
            String title = [[menuItem title] UTF8String];
            String state = [menuItem state] == NSControlStateValueOn ? "true" : "false";
            String parent = [[[menuItem menu] title] UTF8String];
            String seq = std::to_string([menuItem tag]);

            w->eval(getResolveMenuSelectionJavaScript(seq, title, parent));
          }),
        "v@:@:@:"
      );
    }

    objc_setAssociatedObject(
      delegate,
      "webview",
      (id) this,
      OBJC_ASSOCIATION_ASSIGN
    );

    // Initialize application
    [NSApplication sharedApplication];
    [NSApp setActivationPolicy: NSApplicationActivationPolicyRegular];

    if (opts.headless) {
      [NSApp activateIgnoringOtherApps: NO];
    } else {
      // Sets the app as the active app
      [NSApp activateIgnoringOtherApps: YES];
    }

    // Add webview to window
    [window setContentView: webview];

    navigate("0", opts.url);
  }

  ScreenSize CoreWindow::getScreenSize () {
    NSRect e = [[NSScreen mainScreen] frame];

    return ScreenSize {
      .height = (int) e.size.height,
      .width = (int) e.size.width
    };
  }

  void CoreWindow::show (const String& seq) {
    if (this->opts.headless == true) {
      [NSApp activateIgnoringOtherApps: NO];
    } else {
      [window makeKeyAndOrderFront: nil];
      [NSApp activateIgnoringOtherApps: YES];
    }


    if (seq.size() > 0) {
      auto index = std::to_string(this->opts.index);
      this->resolvePromise(seq, "0", index);
    }
  }

  void CoreWindow::exit (int code) {
    if (onExit != nullptr) onExit(code);
  }

  void CoreWindow::kill () {
  }

  void CoreWindow::close (int code) {
    [this->internals->window performClose:nil];
  }

  void CoreWindow::hide (const String& seq) {
    [this->internals->window orderOut: this->internals->window];
    this->eval(javascript::getEmitToRenderProcessJavaScript("windowHide", "{}"));

    if (seq.size() > 0) {
      auto index = std::to_string(this->opts.index);
      this->resolvePromise(seq, "0", index);
    }
  }

  void CoreWindow::eval (const String& js) {
    [webview evaluateJavaScript:
      [NSString stringWithUTF8String:js.c_str()]
      completionHandler:nil];
  }

  void CoreWindow::setSystemMenuItemEnabled (bool enabled, int barPos, int menuPos) {
    NSMenu* menuBar = [NSApp mainMenu];
    NSArray* menuBarItems = [menuBar itemArray];

    NSMenu* menu = menuBarItems[barPos];
    if (!menu) return;

    NSArray* menuItems = [menu itemArray];
    NSMenuItem* menuItem = menuItems[menuPos];

    if (!menuItem) return;

    [menuItem setTarget: nil];
    [menuItem setAction: NULL];
  }

  void CoreWindow::navigate (const String& seq, const String& value) {
    [webview loadRequest:
      [NSURLRequest requestWithURL:
        [NSURL URLWithString:
          [NSString stringWithUTF8String: value.c_str()]]]];

    if (seq.size() > 0) {
      auto index = std::to_string(this->opts.index);
      this->resolvePromise(seq, "0", index);
    }
  }

  void CoreWindow::setTitle (const String& seq, const String& value) {
    [window setTitle:[NSString stringWithUTF8String:value.c_str()]];

    if (seq.size() > 0) {
      auto index = std::to_string(this->opts.index);
      this->resolvePromise(seq, "0", index);
    }
  }

  void CoreWindow::setSize (const String& seq, int width, int height, int hints) {
    [window setFrame:NSMakeRect(0.f, 0.f, (float) width, (float) height) display:YES animate:YES];
    [window center];

    if (seq.size() > 0) {
      auto index = std::to_string(this->opts.index);
      this->resolvePromise(seq, "0", index);
    }
  }

  int CoreWindow::openExternal (const String& s) {
    NSString* nsu = [NSString stringWithUTF8String:s.c_str()];
    return [[NSWorkspace sharedWorkspace] openURL:[NSURL URLWithString: nsu]];
  }

  void CoreWindow::closeContextMenu () {
    // @TODO(jwerle)
  }

  void CoreWindow::closeContextMenu (const String &seq) {
    // @TODO(jwerle)
  }

  void CoreWindow::showInspector () {
    // This is a private method on the webview, so we need to use
    // the pragma keyword to suppress the access warning.
    #pragma clang diagnostic ignored "-Wobjc-method-access"
    [[this->webview _inspector] show];
  }

  void CoreWindow::setBackgroundColor (int r, int g, int b, float a) {
    CGFloat sRGBComponents[4] = { r / 255.0, g / 255.0, b / 255.0, a };
    NSColorSpace *colorSpace = [NSColorSpace sRGBColorSpace];

    [window setBackgroundColor:
      [NSColor colorWithColorSpace: colorSpace
                        components: sRGBComponents
                             count: 4]
    ];
  }

  void CoreWindow::setContextMenu (const String& seq, const String& value) {
    auto menuItems = split(value, '_');
    auto id = std::stoi(seq.substr(1)); // remove the 'R' prefix

    NSPoint mouseLocation = [NSEvent mouseLocation];
    NSMenu *pMenu = [[[NSMenu alloc] initWithTitle:@"contextMenu"] autorelease];
    NSMenuItem *menuItem;
    int index = 0;

    for (auto item : menuItems) {
      if (trim(item).size() == 0) continue;

      if (item.find("---") != -1) {
        NSMenuItem *sep = [NSMenuItem separatorItem];
        [pMenu addItem:sep];
        index++;
        continue;
      }

      auto pair = split(trim(item), ':');

      NSString* nssTitle = [NSString stringWithUTF8String:pair[0].c_str()];
      NSString* nssKey = @"";

      if (pair.size() > 1) {
        nssKey = [NSString stringWithUTF8String:pair[1].c_str()];
      }

      menuItem = [pMenu
        insertItemWithTitle:nssTitle
        action:@selector(menuItemSelected:)
        keyEquivalent:nssKey
        atIndex:index
      ];

      [menuItem setTag:id];

      index++;
    }

    [pMenu
      popUpMenuPositioningItem:pMenu.itemArray[0]
        atLocation:NSPointFromCGPoint(CGPointMake(mouseLocation.x, mouseLocation.y))
        inView:nil];
  }

  void CoreWindow::setSystemMenu (const String& seq, const String& value) {
    String menu = String(value);

    NSMenu *mainMenu;
    NSString *title;
    NSMenu *appleMenu;
    NSMenu *serviceMenu;
    NSMenu *windowMenu;
    NSMenu *editMenu;
    NSMenu *dynamicMenu;
    NSMenuItem *menuItem;

    if (NSApp == nil) return;

    mainMenu = [[NSMenu alloc] init];

    // Create the main menu bar
    [NSApp setMainMenu:mainMenu];
    [mainMenu release];
    mainMenu = nil;

    // Create the application menu

    // id appName = [[NSProcessInfo processInfo] processName];
    // title = [@"About " stringByAppendingString:appName];

    // deserialize the menu
    menu = replace(menu, "%%", "\n");

    // split on ;
    auto menus = split(menu, ';');

    for (auto m : menus) {
      auto menu = split(m, '\n');
      auto line = trim(menu[0]);
      if (line.empty()) continue;
      auto menuTitle = split(line, ':')[0];
      NSString* nssTitle = [NSString stringWithUTF8String:menuTitle.c_str()];
      dynamicMenu = [[NSMenu alloc] initWithTitle:nssTitle];
      bool isDisabled = false;

      for (int i = 1; i < menu.size(); i++) {
        auto line = trim(menu[i]);
        if (line.empty()) continue;
        auto parts = split(line, ':');
        auto title = parts[0];
        NSUInteger mask = 0;
        String key = "";

        if (title.size() > 0 && title.find("!") == 0) {
          title = title.substr(1);
          isDisabled = true;
        }

        if (parts.size() == 2) {
          if (parts[1].find("+") != -1) {
            auto accelerator = split(parts[1], '+');
            key = trim(accelerator[0]);

            if (accelerator[1].find("CommandOrControl") != -1) {
              mask |= NSEventModifierFlagCommand;
            } else if (accelerator[1].find("Meta") != -1) {
              mask |= NSEventModifierFlagCommand;
            } else if (accelerator[1].find("Control") != -1) {
              mask |= NSEventModifierFlagControl;
            }

            if (accelerator[1].find("Alt") != -1) {
              mask |= NSEventModifierFlagOption;
            }
          } else {
            key = trim(parts[1]);
          }
        }

        NSString* nssTitle = [NSString stringWithUTF8String:title.c_str()];
        NSString* nssKey = [NSString stringWithUTF8String:key.c_str()];
        NSString* nssSelector = [NSString stringWithUTF8String:"menuItemSelected:"];

        if (menuTitle.compare("Edit") == 0) {
          if (title.compare("Cut") == 0) nssSelector = [NSString stringWithUTF8String:"cut:"];
          if (title.compare("Copy") == 0) nssSelector = [NSString stringWithUTF8String:"copy:"];
          if (title.compare("Paste") == 0) nssSelector = [NSString stringWithUTF8String:"paste:"];
          if (title.compare("Delete") == 0) nssSelector = [NSString stringWithUTF8String:"delete:"];
          if (title.compare("Select All") == 0) nssSelector = [NSString stringWithUTF8String:"selectAll:"];
        }

        if (title.find("About") == 0) {
          nssSelector = [NSString stringWithUTF8String:"orderFrontStandardAboutPanel:"];
        }

        if (title.find("Hide") == 0) {
          nssSelector = [NSString stringWithUTF8String:"hide:"];
        }

        if (title.find("Hide Others") == 0) {
          nssSelector = [NSString stringWithUTF8String:"hideOtherApplications:"];
        }

        if (title.find("Show All") == 0) {
          nssSelector = [NSString stringWithUTF8String:"unhideAllApplications:"];
        }

        if (title.find("Quit") == 0) {
          // nssSelector = [NSString stringWithUTF8String:"terminate:"];
        }

        if (title.compare("Minimize") == 0) {
          nssSelector = [NSString stringWithUTF8String:"performMiniaturize:"];
        }

        // if (title.compare("Zoom") == 0) nssSelector = [NSString stringWithUTF8String:"performZoom:"];

        if (title.find("---") != -1) {
          NSMenuItem *sep = [NSMenuItem separatorItem];
          [dynamicMenu addItem:sep];
        } else {
          menuItem = [dynamicMenu
            addItemWithTitle: nssTitle
            action: NSSelectorFromString(nssSelector)
            keyEquivalent: nssKey
          ];
        }

        if (mask != 0) {
          [menuItem setKeyEquivalentModifierMask: mask];
        }

        if (isDisabled) {
          [menuItem setTarget:nil];
          [menuItem setAction:NULL];
        }

        [menuItem setTag:0]; // only contextMenu uses the tag
      }

      menuItem = [[NSMenuItem alloc] initWithTitle:nssTitle action:nil keyEquivalent:@""];

      [[NSApp mainMenu] addItem:menuItem];
      [menuItem setSubmenu:dynamicMenu];
      [menuItem release];
      [dynamicMenu release];
    }

    if (seq.size() > 0) {
      auto index = std::to_string(this->opts.index);
      this->resolvePromise(seq, "0", index);
    }
  }

  void CoreWindow::openDialog (
    const String& seq,
    bool isSave,
    bool allowDirs,
    bool allowFiles,
    bool allowMultiple,
    const String& defaultPath = "",
    const String& title = "",
    const String& defaultName = "")
  {

    NSURL *url;
    const char *utf8_path;
    NSSavePanel *dialog_save;
    NSOpenPanel *dialog_open;
    NSURL *default_url = nil;
    NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];

    if (isSave) {
      dialog_save = [NSSavePanel savePanel];
      [dialog_save setTitle: [NSString stringWithUTF8String:title.c_str()]];
    } else {
      dialog_open = [NSOpenPanel openPanel];
      // open does not support title for some reason
    }

    if (!isSave) {
      [dialog_open setCanCreateDirectories:YES];
    }

    if (allowDirs == true && isSave == false) {
      [dialog_open setCanChooseDirectories:YES];
    }

    if (isSave == false) {
      [dialog_open setCanChooseFiles: allowFiles ? YES : NO];
    }

    if ((isSave == false || allowDirs == true) && allowMultiple == true) {
      [dialog_open setAllowsMultipleSelection:YES];
    }

    if (defaultName.size() > 0) {
      default_url = [NSURL fileURLWithPath:
        [NSString stringWithUTF8String: defaultName.c_str()]];
    } else if (defaultPath.size() > 0) {
      default_url = [NSURL fileURLWithPath:
        [NSString stringWithUTF8String: defaultPath.c_str()]];
    }

    if (default_url != nil) {
      if (isSave) {
        [dialog_save setDirectoryURL: default_url];
        [dialog_save setNameFieldStringValue: default_url.lastPathComponent];
      } else {
        [dialog_open setDirectoryURL: default_url];
        [dialog_open setNameFieldStringValue: default_url.lastPathComponent];
      }
    }

    if (title.size() > 0) {
      NSString* nssTitle = [NSString stringWithUTF8String:title.c_str()];
      if (isSave) {
        [dialog_save setTitle: nssTitle];
      } else {
        [dialog_open setTitle: nssTitle];
      }
    }

    if (isSave) {
      if ([dialog_save runModal] == NSModalResponseOK) {
        String url = (char*) [[[dialog_save URL] path] UTF8String];
        auto wrapped = String("\"" + url + "\"");
        this->resolvePromise(seq, "0", encodeURIComponent(wrapped));
      }
      return;
    }

    String result = "";
    ssc::Vector<String> paths;
    NSArray* urls;

    if ([dialog_open runModal] == NSModalResponseOK) {
      urls = [dialog_open URLs];

      for (NSURL* url in urls) {
        if ([url isFileURL]) {
          paths.push_back(String((char*) [[url path] UTF8String]));
        }
      }
    }

    [pool release];

    for (size_t i = 0, i_end = paths.size(); i < i_end; ++i) {
      result += (i ? "\\n" : "");
      result += paths[i];
    }

    auto wrapped = String("\"" + result + "\"");
    this->resolvePromise(seq, "0", encodeURIComponent(wrapped));
  }
}
#endif
