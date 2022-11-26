#ifndef SSC_SOCKET_WINDOW_HH
#define SSC_SOCKET_WINDOW_HH

#include "common.hh"
#include "runtime.hh"
#include "utils.hh"
#include "webview.hh"

namespace ssc::runtime::window {
  using namespace runtime;
  using namespace webview;

  using config::Config;
  using utils::encodeURIComponent;

  // forward
  class DragDrop;
  class WindowInternals;

  #if defined(SSC_MAX_WINDOWS)
    static constexpr int MAX_WINDOWS = SSC_MAX_WINDOWS;
  #else
    static constexpr int MAX_WINDOWS = 32;
  #endif

  struct ScreenSize {
    size_t height = 0;
    size_t width = 0;
  };

  enum {
    WINDOW_HINT_NONE = 0,  // Width and height are default size
    WINDOW_HINT_MIN = 1,   // Width and height are minimum bounds
    WINDOW_HINT_MAX = 2,   // Width and height are maximum bounds
    WINDOW_HINT_FIXED = 3  // Window size can not be changed by a user
  };

  struct WindowOptions {
    bool resizable = true;
    bool frameless = false;
    bool utility = false;
    bool canExit = false;
    float height = 0;
    float width = 0;
    bool isHeightInPercent = false;
    bool isWidthInPercent = false;
    int index = 0;
    int debug = 0;
    int port = 0;
    bool isTest = false;
    bool headless = false;
    bool forwardConsole = false;
    String cwd = "";
    String executable = "";
    String title = "";
    String url = "data:text/html,<html>";
    String version = "";
    String argv = "";
    String preload = "";
    String env;
    Config config;
    MessageCallback onMessage = [](const String) { return false; };
    ExitCallback onExit = nullptr;
    DataManager* dataManager = nullptr;
  };

  inline String createPreload (WindowOptions opts) {
    static PlatformInfo platform;
    auto cwd = String(opts.cwd);
    std::replace(cwd.begin(), cwd.end(), '\\', '/');

    auto preload = String(
      "\n;(() => {                                                    \n"
      "window.__args = {                                              \n"
      "  arch: '" + platform.arch + "',                               \n"
      "  cwd: () => '" + cwd + "',                                    \n"
      "  debug: " + std::to_string(opts.debug) + ",              \n"
      "  config: {},                                                  \n"
      "  executable: '" + opts.executable + "',                       \n"
      "  index: Number('" + std::to_string(opts.index) + "'),    \n"
      "  os: '" + platform.os + "',                                   \n"
      "  platform: '" + platform.os + "',                             \n"
      "  port: Number('" + std::to_string(opts.port) + "'),      \n"
      "  title: '" + opts.title + "',                                 \n"
      "  env: {},                                                     \n"
      "  version: '" + opts.version + "',                             \n"
      "};                                                             \n"
      "Object.assign(                                                 \n"
      "  window.__args.env,                                           \n"
      "  Object.fromEntries(new URLSearchParams('" +  opts.env + "')) \n"
      ");                                                             \n"
      "window.__args.argv = [" + opts.argv + "];                      \n"
    );

    if (platform.mac || platform.linux || platform.win) {
      preload += "                                                      \n"
        "if (window?.parent?.port > 0) {                                \n"
        "  window.addEventListener('menuItemSelected', e => {           \n"
        "    window.location.reload();                                  \n"
        "  });                                                          \n"
        "}                                                              \n"
        "const uri = `ipc://process.open`;                              \n"
        "if (window?.webkit?.messageHandlers?.external?.postMessage) {  \n"
        "  window.webkit.messageHandlers.external.postMessage(uri);     \n"
        "} else if (window?.chrome?.webview?.postMessage) {             \n"
        "  window.chrome.webview.postMessage(uri);                      \n"
        "}                                                              \n";
    }

    for (auto const &tuple : opts.config.entries) {
      auto key = trim(tuple.first);
      auto value = trim(tuple.second);

      // skip empty key/value and comments
      if (key.size() == 0 || value.size() == 0 || key.rfind("#", 0) == 0) {
        continue;
      }

      preload += "  ;(() => { \n";
      preload += "    const key = decodeURIComponent('" + encodeURIComponent(key) + "').toLowerCase()\n";

      if (value == "true" || value == "false") {
        preload += "    window.__args.config[key] = " + value + "\n";
      } else {
        preload += "    const value = '" + encodeURIComponent(value) + "'\n";
        preload += "    if (!Number.isNaN(parseFloat(value))) {\n";
        preload += "      window.__args.config[key] = parseFloat(value);\n";
        preload += "    } else { \n";
        preload += "      let val = decodeURIComponent(value);\n";
        preload += "      try { val = JSON.parse(val) } catch (err) {}\n";
        preload += "      window.__args.config[key] = val;\n";
        preload += "    }\n";
      }

      preload += "  })();\n";
    }

    preload += "  Object.seal(Object.freeze(window.__args.config));\n";

    // deprecate usage of 'window.system' in favor of 'window.__args'
    preload += String(
      "  Object.defineProperty(window, 'system', {         \n"
      "    configurable: false,                            \n"
      "    enumerable: false,                              \n"
      "    writable: false,                                \n"
      "    value: new Proxy(window.__args, {               \n"
      "      get (target, prop, receiver) {                \n"
      "        console.warn(                               \n"
      "          `window.system.${prop} is deprecated. ` + \n"
      "          `Use window.__args.${prop} instead.`      \n"
      "         );                                         \n"
      "        return Reflect.get(...arguments);           \n"
      "      }                                             \n"
      "    })                                              \n"
      "  });                                               \n"
    );

    preload += "})();\n";
    return preload;
  }

  inline String getResolveToMainProcessMessage (
    const String& seq,
    const String& state,
    const String& value
  ) {
    return String("ipc://resolve?seq=" + seq + "&state=" + state + "&value=" + value);
  }

  #if defined(_WIN32)
    using IEnvHandler = ICoreWebView2CreateCoreWebView2EnvironmentCompletedHandler;
    using IConHandler = ICoreWebView2CreateCoreWebView2ControllerCompletedHandler;
    using INavHandler = ICoreWebView2NavigationCompletedEventHandler;
    using IRecHandler = ICoreWebView2WebMessageReceivedEventHandler;
    using IArgs = ICoreWebView2WebMessageReceivedEventArgs;

	  // constexpr COLORREF darkBkColor = 0x383838;
	  // constexpr COLORREF darkTextColor = 0xFFFFFF;
	  // static HBRUSH hbrBkgnd = nullptr;

    enum WINDOWCOMPOSITIONATTRIB {
      WCA_UNDEFINED = 0,
      WCA_NCRENDERING_ENABLED = 1,
      WCA_NCRENDERING_POLICY = 2,
      WCA_TRANSITIONS_FORCEDISABLED = 3,
      WCA_ALLOW_NCPAINT = 4,
      WCA_CAPTION_BUTTON_BOUNDS = 5,
      WCA_NONCLIENT_RTL_LAYOUT = 6,
      WCA_FORCE_ICONIC_REPRESENTATION = 7,
      WCA_EXTENDED_FRAME_BOUNDS = 8,
      WCA_HAS_ICONIC_BITMAP = 9,
      WCA_THEME_ATTRIBUTES = 10,
      WCA_NCRENDERING_EXILED = 11,
      WCA_NCADORNMENTINFO = 12,
      WCA_EXCLUDED_FROM_LIVEPREVIEW = 13,
      WCA_VIDEO_OVERLAY_ACTIVE = 14,
      WCA_FORCE_ACTIVEWINDOW_APPEARANCE = 15,
      WCA_DISALLOW_PEEK = 16,
      WCA_CLOAK = 17,
      WCA_CLOAKED = 18,
      WCA_ACCENT_POLICY = 19,
      WCA_FREEZE_REPRESENTATION = 20,
      WCA_EVER_UNCLOAKED = 21,
      WCA_VISUAL_OWNER = 22,
      WCA_HOLOGRAPHIC = 23,
      WCA_EXCLUDED_FROM_DDA = 24,
      WCA_PASSIVEUPDATEMODE = 25,
      WCA_USEDARKMODECOLORS = 26,
      WCA_LAST = 27
    };

    struct WINDOWCOMPOSITIONATTRIBDATA {
      WINDOWCOMPOSITIONATTRIB Attrib;
      PVOID pvData;
      SIZE_T cbData;
    };
  #endif

  class Window {
    public:
      WindowOptions opts;
      CoreApplication& app;
      Runtime& runtime;
      Bridge bridge;
      WindowInternals *internals = nullptr;
      DataManager* dataManager = nullptr;
      WebView* webview = nullptr;

      MessageCallback onMessage = [](const String) { return false; };
      ExitCallback onExit = nullptr;
      int index = 0;

      Window () = delete;
      Window (
        CoreApplication&,
        Runtime& runtime,
        const WindowOptions
      );

      void initialize ();
      void about ();
      void eval (const Script);
      void eval (const String);
      void show (const String&);
      void hide (const String&);
      void kill ();
      void exit (int code);
      void close (int code);
      void navigate (const String&, const String&);
      void setTitle (const String&, const String&);
      void setSize (const String&, int, int, int);
      void setContextMenu (const String&, const String&);
      void closeContextMenu (const String&);
      void closeContextMenu ();
      void setBackgroundColor (int r, int g, int b, float a);
      void setSystemMenuItemEnabled (bool enabled, int barPos, int menuPos);
      void setSystemMenu (const String& seq, const String& menu);
      ScreenSize getScreenSize ();
      void showInspector ();
      int openExternal (const String& s);
      void openDialog ( // @TODO(jwerle): use `OpenDialogOptions` here instead
        const String&,
        bool,
        bool,
        bool,
        bool,
        const String&,
        const String&,
        const String&
      );

      void blur ();
      void focus ();

      void resolvePromise (
        const String& seq,
        const String& state,
        const String& value
      );

      void dispatchEvent (
        const String& name,
        const String& data
      );


      bool onIPCSchemeRequestRouteCallback (webview::SchemeRequest& request);
      bool onScriptMessage (const String& string);
  };

  struct WindowManagerOptions {
    float defaultHeight = 0;
    float defaultWidth = 0;
    bool isHeightInPercent = false;
    bool isWidthInPercent = false;
    bool headless = false;
    bool isTest;
    String argv = "";
    String cwd = "";
    Config config;
    MessageCallback onMessage = [](const String) { return false; };
    ExitCallback onExit = nullptr;
    DataManager* dataManager = nullptr;
  };

  class WindowManager  {
    public:
      enum WindowStatus {
        WINDOW_ERROR = -1,
        WINDOW_NONE = 0,
        WINDOW_CREATING = 10,
        WINDOW_CREATED,
        WINDOW_HIDING = 20,
        WINDOW_HIDDEN,
        WINDOW_SHOWING = 30,
        WINDOW_SHOWN,
        WINDOW_CLOSING = 40,
        WINDOW_CLOSED,
        WINDOW_EXITING = 50,
        WINDOW_EXITED,
        WINDOW_KILLING = 60,
        WINDOW_KILLED
      };

      class ManagedWindow : public Window {
        public:
          WindowStatus status = WINDOW_NONE;
          WindowManager &manager;

          ManagedWindow (
            WindowManager& manager,
            CoreApplication& app,
            Runtime& runtime,
            WindowOptions opts
          );

          void show (const String &seq);
          void hide (const String &seq);
          void close (int code);
          void exit (int code);
          void kill ();
          void gc ();
      };

      std::chrono::system_clock::time_point lastDebugLogLine;

      CoreApplication &app;
      bool destroyed = false;
      std::vector<bool> inits;
      std::vector<ManagedWindow*> windows;
      std::recursive_mutex mutex;
      WindowManagerOptions options;
      Runtime& runtime;

      WindowManager (
        CoreApplication& app,
        Runtime& runtime
      );

      ~WindowManager ();

      void destroy ();
      void configure (WindowManagerOptions configuration);
      void log (const String line);

      Window* getWindow (int index, WindowStatus status);
      Window* getWindow (int index);
      Window* getOrCreateWindow (int index);
      Window* getOrCreateWindow (int index, WindowOptions opts);
      WindowStatus getWindowStatus (int index);
      void destroyWindow (int index);
      void destroyWindow (ManagedWindow* window);
      void destroyWindow (Window* window);
      Window* createWindow (WindowOptions opts);
      Window* createDefaultWindow (WindowOptions opts);
  };
}

#if defined(__APPLE__)
  #if TARGET_OS_IPHONE || TARGET_IPHONE_SIMULATOR
    @interface CoreWindowDelegate : NSObject<WKScriptMessageHandler, UIScrollViewDelegate >
    @property (nonatomic) ssc::runtime::window::WindowInternals* internals;
    @property (strong, nonatomic) CoreWKWebView* webview;
    @end
  #else
    @interface CoreWindowDelegate : NSObject <NSWindowDelegate, WKScriptMessageHandler>
    @property (nonatomic) ssc::runtime::window::WindowInternals* internals;
    @property (strong, nonatomic) CoreWKWebView* webview;
    - (void) userContentController: (WKUserContentController*) userContentController
           didReceiveScriptMessage: (WKScriptMessage*) scriptMessage;
    @end
  #endif
#endif

namespace ssc::runtime::window {
  class WindowInternals {
    public:
      Window* window;
    #if defined(__APPLE__)
      CoreWindowDelegate* coreDelegate = nullptr;
      #if TARGET_OS_IPHONE || TARGET_IPHONE_SIMULATOR
        UIWindow* coreWindow = nullptr;
      #else
        NSWindow* coreWindow = nullptr;
      #endif
    #elif defined(__linux__) && !defined(__ANDROID__)
      GtkSelectionData *selectionData = nullptr;
      GtkAccelGroup *accelGroup = nullptr;
      GtkWidget *webview = nullptr;
      GtkWidget *window = nullptr;
      GtkWidget *menubar = nullptr;
      GtkWidget *vbox = nullptr;
      GtkWidget *popup = nullptr;
      std::vector<String> draggablePayload;
      double dragLastX = 0;
      double dragLastY = 0;
      bool isDragInvokedInsideWindow;
      int popupId;
    #elif defined(_WIN32)
      static LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
      ICoreWebView2Controller *controller = nullptr;
      ICoreWebView2 *webview = nullptr;
      HMENU systemMenu;
      DWORD mainThread = GetCurrentThreadId();
      POINT m_minsz = POINT {0, 0};
      POINT m_maxsz = POINT {0, 0};
      DragDrop* drop;
      HWND window;
      std::map<int, std::string> menuMap;
      void resize (HWND window);
    #endif

      WindowInternals (
        Window* coreWindow,
        const WindowOptions& opts
      );

    #if defined(__linux__) && !defined(__ANDROID__)
      void closeContextMenu (GtkWidget *, const String&);
    #endif
  };
}
#endif
