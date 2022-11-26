#include <socket/window.hh>

namespace JSON = ssc::JSON;

namespace ssc::runtime::window {
  static bool isDelegateSet = false;

  WindowInternals::WindowInternals (
    Window* window,
    const WindowOptions& opts
  ) {
    this->window = window;
    this->coreDelegate = [CoreWindowDelegate new];
    this->coreDelegate.internals = this;

    #if TARGET_OS_IPHONE || TARGET_IPHONE_SIMULATOR
      auto notificationCenter = [NSNotificationCenter defaultCenter];
      auto viewController = [[UIViewController alloc] init];
      auto frame = [[UIScreen mainScreen] bounds];

      this->coreWindow = [[UIWindow alloc] initWithFrame: frame];
      this->coreWindow.rootViewController = viewController;

      [viewController.view setFrame: frame];

      [notificationCenter
        addObserver: this->coreDelegate
           selector: @selector(keyboardDidShow)
               name: UIKeyboardDidShowNotification
              object: nil
      ];

      [notificationCenter
        addObserver: this->coreDelegate
           selector: @selector(keyboardDidHide)
               name: UIKeyboardDidHideNotification
             object: nil
      ];

      [notificationCenter
        addObserver: this->coreDelegate
           selector: @selector(keyboardWillShow)
               name: UIKeyboardWillShowNotification
             object: nil
      ];

      [notificationCenter
        addObserver: this->coreDelegate
          selector: @selector(keyboardWillHide)
              name: UIKeyboardWillHideNotification
            object: nil
      ];

      [notificationCenter
        addObserver: this->coreDelegate
           selector: @selector(keyboardWillChange:)
               name: UIKeyboardWillChangeFrameNotification
             object: nil
      ];
    #else
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

      auto screenSize = window->getScreenSize();
      auto height = opts.isHeightInPercent ? screenSize.height * opts.height / 100 : opts.height;
      auto width = opts.isWidthInPercent ? screenSize.width * opts.width / 100 : opts.width;

      this->coreWindow = [[NSWindow alloc]
          initWithContentRect: NSMakeRect(0, 0, width, height)
                    styleMask: style
                      backing: NSBackingStoreBuffered
                        defer: NO
      ];

      // Position window in center of screen
      [this->coreWindow center];
      [this->coreWindow setOpaque: YES];
      // Minimum window size
      [this->coreWindow setContentMinSize: NSMakeSize(width, height)];
      [this->coreWindow setBackgroundColor: [NSColor controlBackgroundColor]];
      [this->coreWindow registerForDraggedTypes: draggableTypes];
      // [this->coreWindow setAppearance:[NSAppearance appearanceNamed:NSAppearanceNameVibrantDark]];
      // [this->coreWindow setMovableByWindowBackground: true];

      if (opts.frameless) {
        [this->coreWindow setTitlebarAppearsTransparent: true];
      }

      [this->coreWindow setDelegate: this->coreDelegate];
    #endif
  }

  void Window::initialize ()  {
    #if TARGET_OS_IPHONE || TARGET_IPHONE_SIMULATOR
      [this->webview->internals->coreWebView.scrollView setDelegate: this->internals->coreDelegate];
    #endif

    /*
     TODO(@jwerle): move this to bridge
    this->bridge->router.map("window.eval", [=](auto message, auto router, auto reply) {
      auto value = message.value;
      auto seq = message.seq;
      auto  script = [NSString stringWithUTF8String: value.c_str()];

      dispatch_async(dispatch_get_main_queue(), ^{
        [this->internals->coreWebView evaluateJavaScript: script completionHandler: ^(id result, NSError *error) {
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
    */

    bool exiting = false;

    if (!isDelegateSet) {
      isDelegateSet = true;

      class_replaceMethod(
        [CoreWindowDelegate class],
        @selector(userContentController:didReceiveScriptMessage:),
        imp_implementationWithBlock(
          [=](id self, SEL cmd, WKScriptMessage* scriptMessage) {
            auto window = (__bridge Window*) objc_getAssociatedObject(self, "window");
            id body = [scriptMessage body];

            if (![body isKindOfClass:[NSString class]]) {
              return;
            }

            String msg = [body UTF8String];

            window->onScriptMessage(String([body UTF8String]));
          }),
        "v@:@"
      );

      #if !TARGET_OS_IPHONE && !TARGET_IPHONE_SIMULATOR
        class_replaceMethod(
          [CoreWindowDelegate class],
          @selector(windowShouldClose:),
          imp_implementationWithBlock(
            [&](id self, SEL cmd, id notification) {
              if (exiting) return true;

              auto window = (Window*) objc_getAssociatedObject(self, "window");

              if (window->opts.canExit) {
                exiting = true;
                window->exit(0);
                return true;
              }

              window->eval(getEmitToRenderProcessJavaScript("windowHide", "{}"));
              window->hide("");
              return false;
            }),
          "v@:@"
        );

        class_addMethod(
          [CoreWindowDelegate class],
          @selector(menuItemSelected:),
          imp_implementationWithBlock(
            [=](id self, SEL _cmd, id item) {
              auto window = (Window*) objc_getAssociatedObject(self, "window");
              if (window->onMessage != nullptr) {
                id menuItem = (id) item;
                String title = [[menuItem title] UTF8String];
                String state = [menuItem state] == NSControlStateValueOn ? "true" : "false";
                String parent = [[[menuItem menu] title] UTF8String];
                String seq = std::to_string([menuItem tag]);

                window->eval(getResolveMenuSelectionJavaScript(seq, title, parent));
              }
            }),
          "v@:@:@:"
        );
      #endif
    }

    objc_setAssociatedObject(
      this->internals->coreDelegate,
      "window",
      (__bridge id) this,
      OBJC_ASSOCIATION_ASSIGN
    );

    #if TARGET_OS_IPHONE || TARGET_IPHONE_SIMULATOR
      [this->internals->coreWindow.rootViewController.view
        addSubview: this->webview->internals->coreWebView
      ];

      [this->internals->coreWindow makeKeyAndVisible];
    #else
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
      [this->internals->coreWindow setContentView: this->webview->internals->coreWebView];
    #endif

    this->navigate("", opts.url);
  }

  ScreenSize Window::getScreenSize () {
    #if !TARGET_OS_IPHONE && !TARGET_IPHONE_SIMULATOR
      NSRect e = [[NSScreen mainScreen] frame];

      return ScreenSize {
        .height = (size_t) e.size.height,
        .width = (size_t) e.size.width
      };
    #else
      // TODO
      return ScreenSize { 0, 0 };
    #endif
  }

  void Window::show (const String& seq) {
    #if !TARGET_OS_IPHONE && !TARGET_IPHONE_SIMULATOR
      if (this->opts.headless == true) {
        [NSApp activateIgnoringOtherApps: NO];
      } else {
        [this->internals->coreWindow makeKeyAndOrderFront: nil];
        [NSApp activateIgnoringOtherApps: YES];
      }
    #endif

    if (seq.size() > 0) {
      auto index = std::to_string(this->opts.index);
      this->resolvePromise(seq, "0", index);
    }
  }

  void Window::kill () {
  }

  void Window::close (int code) {
    #if !TARGET_OS_IPHONE && !TARGET_IPHONE_SIMULATOR
      // `onExit` handled in `windowShouldClose:` selector
      [this->internals->coreWindow performClose: nil];
    #endif
  }

  void Window::hide (const String& seq) {
    #if !TARGET_OS_IPHONE && !TARGET_IPHONE_SIMULATOR
      [this->internals->coreWindow orderOut: this->internals->coreWindow];
    #endif

    this->eval(getEmitToRenderProcessJavaScript("windowHide", "{}"));

    if (seq.size() > 0) {
      auto index = std::to_string(this->opts.index);
      this->resolvePromise(seq, "0", index);
    }
  }

  void Window::eval (const String js) {
    [this->webview->internals->coreWebView
      evaluateJavaScript: [NSString stringWithUTF8String: js.c_str()]
        completionHandler: nil
    ];
  }

  void Window::setSystemMenuItemEnabled (bool enabled, int barPos, int menuPos) {
    #if !TARGET_OS_IPHONE && !TARGET_IPHONE_SIMULATOR
      auto menuBar = [NSApp mainMenu];
      auto menuBarItems = [menuBar itemArray];
      auto menu = (NSMenu*) menuBarItems[barPos];
      if (menu) {
        auto menuItems = [menu itemArray];
        auto menuItem = menuItems[menuPos];
        if (menuItem) {
          [menuItem setTarget: nil];
          [menuItem setAction: NULL];
        }
      }
    #endif
  }

  void Window::navigate (const String& seq, const String& value) {
    #if TARGET_OS_IPHONE || TARGET_IPHONE_SIMULATOR
      auto allowed = [[NSBundle mainBundle] resourcePath];
      auto url = [allowed stringByAppendingPathComponent:@"ui/index.html"];
      [this->webview->internals->coreWebView
        loadFileURL: [NSURL fileURLWithPath: url]
        allowingReadAccessToURL: [NSURL fileURLWithPath: allowed]
      ];
    #else
      [this->webview->internals->coreWebView
        loadRequest: [NSURLRequest
          requestWithURL: [NSURL
            URLWithString: [NSString stringWithUTF8String: value.c_str()]
          ]
        ]
      ];
    #endif

    if (seq.size() > 0) {
      auto index = std::to_string(this->opts.index);
      this->resolvePromise(seq, "0", index);
    }
  }

  void Window::setTitle (const String& seq, const String& value) {
    #if !TARGET_OS_IPHONE && !TARGET_IPHONE_SIMULATOR
      [this->internals->coreWindow setTitle: [NSString stringWithUTF8String: value.c_str()]];
    #endif

    if (seq.size() > 0) {
      auto index = std::to_string(this->opts.index);
      this->resolvePromise(seq, "0", index);
    }
  }

  void Window::setSize (const String& seq, int width, int height, int hints) {
    #if !TARGET_OS_IPHONE && !TARGET_IPHONE_SIMULATOR
      [this->internals->coreWindow
          setFrame: NSMakeRect(0.f, 0.f, (float) width, (float) height)
          display: YES
          animate: YES
      ];

      [this->internals->coreWindow center];
    #endif

    if (seq.size() > 0) {
      auto index = std::to_string(this->opts.index);
      this->resolvePromise(seq, "0", index);
    }
  }

  int Window::openExternal (const String& value) {
    auto string = [NSString stringWithUTF8String: value.c_str()];
    auto url = [NSURL URLWithString: string];

    #if TARGET_OS_IPHONE || TARGET_IPHONE_SIMULATOR
      auto sharedApplication = [UIApplication sharedApplication];
      [sharedApplication openURL: url options: @{} completionHandler: nil];
      return 0;
    #else
      return [[NSWorkspace sharedWorkspace] openURL: url];
    #endif
  }

  void Window::closeContextMenu () {
    // @TODO(jwerle)
  }

  void Window::closeContextMenu (const String &seq) {
    // @TODO(jwerle)
  }

  void Window::showInspector () {
    // This is a private method on the webview, so we need to use
    // the pragma keyword to suppress the access warning.
    #if !TARGET_OS_IPHONE && !TARGET_IPHONE_SIMULATOR
      #pragma clang diagnostic ignored "-Wobjc-method-access"
      [[this->webview->internals->coreWebView _inspector] show];
    #endif
  }

  void Window::setBackgroundColor (int r, int g, int b, float a) {
    #if !TARGET_OS_IPHONE && !TARGET_IPHONE_SIMULATOR
      CGFloat sRGBComponents[4] = { r / 255.0, g / 255.0, b / 255.0, a };
      NSColorSpace *colorSpace = [NSColorSpace sRGBColorSpace];

      [this->internals->coreWindow
        setBackgroundColor: [NSColor
          colorWithColorSpace: colorSpace
                   components: sRGBComponents
                        count: 4
        ]
      ];
    #endif
  }

  void Window::setContextMenu (const String& seq, const String& value) {
    #if !TARGET_OS_IPHONE && !TARGET_IPHONE_SIMULATOR
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
        popUpMenuPositioningItem: pMenu.itemArray[0]
                      atLocation: NSPointFromCGPoint(CGPointMake(mouseLocation.x, mouseLocation.y))
                          inView: nil
      ];
    #endif
  }

  void Window::setSystemMenu (const String& seq, const String& value) {
    #if defined(__APPLE__) && TARGET_OS_MAC && !TARGET_OS_IPHONE && !TARGET_IPHONE_SIMULATOR
      auto menu = String(value);

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
    #else
      if (seq.size() > 0) {
        auto value = JSON::Object(JSON::Object::Entries {
          {"err", JSON::Object::Entries {
            {"message", "Window::setSystemMenu is not supported on this platform"}
          }}
        });

        this->resolvePromise(seq, "0", value.str());
      }
    #endif
  }

  void Window::openDialog (
    const String& seq,
    bool isSave,
    bool allowDirs,
    bool allowFiles,
    bool allowMultiple,
    const String& defaultPath = "",
    const String& title = "",
    const String& defaultName = "")
  {
    #if !TARGET_OS_IPHONE && !TARGET_IPHONE_SIMULATOR
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
      Vector<String> paths;
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
    #endif
  }
}

@implementation CoreWindowDelegate
#if TARGET_OS_IPHONE || TARGET_IPHONE_SIMULATOR
- (void) scrollViewDidScroll: (UIScrollView*) scrollView {
  scrollView.bounds = self.webview.bounds;
}

- (void) keyboardWillHide {
  auto json = JSON::Object::Entries {
    {"value", JSON::Object::Entries {
      {"event", "will-hide"}
    }}
  };

  self.webview.scrollView.scrollEnabled = YES;
  self.internals->window->dispatchEvent("keyboard", JSON::Object(json).str());
}

- (void) keyboardDidHide {
  auto json = JSON::Object::Entries {
    {"value", JSON::Object::Entries {
      {"event", "did-hide"}
    }}
  };

  self.internals->window->dispatchEvent("keyboard", JSON::Object(json).str());
}
- (void) keyboardWillShow {
  auto json = JSON::Object::Entries {
    {"value", JSON::Object::Entries {
      {"event", "will-show"}
    }}
  };

  self.webview.scrollView.scrollEnabled = NO;
  self.internals->window->dispatchEvent("keyboard", JSON::Object(json).str());
}
- (void) keyboardDidShow {
  auto json = JSON::Object::Entries {
    {"value", JSON::Object::Entries {
      {"event", "did-show"}
    }}
  };

  self.internals->window->dispatchEvent("keyboard", JSON::Object(json).str());
}
- (void) keyboardWillChange: (NSNotification*) notification {
  NSDictionary* keyboardInfo = [notification userInfo];
  NSValue* keyboardFrameBegin = [keyboardInfo valueForKey: UIKeyboardFrameEndUserInfoKey];
  CGRect rect = [keyboardFrameBegin CGRectValue];
  CGFloat width = rect.size.width;
  CGFloat height = rect.size.height;

  auto json = JSON::Object::Entries {
    {"value", JSON::Object::Entries {
      {"event", "will-change"},
      {"width", width},
      {"height", height},
    }}
  };

  self.internals->window->dispatchEvent("keyboard", JSON::Object(json).str());
}
#endif

- (void) userContentController: (WKUserContentController*) userContentController
       didReceiveScriptMessage: (WKScriptMessage*) scriptMessage
{
  // overloaded with `class_replaceMethod()`
}
@end

