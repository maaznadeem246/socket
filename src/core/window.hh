#ifndef SSC_CORE_WINDOW_HH
#define SSC_CORE_WINDOW_HH

#include <socket/platform.hh>
#include "application.hh"
#include "config.hh"
#include "data.hh"
#include "string.hh"
#include "types.hh"
#include "utils.hh"
#include "webview.hh"

namespace ssc::core::window {
  using application::CoreApplication;
  using utils::encodeURIComponent;
  using config::Config;
  using data::CoreDataManager;
  using string::String;
  using string::trim;
  using types::ExitCallback;
  using types::Map;
  using types::MessageCallback;
  using webview::CoreWebView;

  // forward
  class DragDrop;
  class CoreWindowInternals;

  #if !defined(MAX_WINDOWS)
    #if defined(SSC_MAX_WINDOWS)
      static constexpr int MAX_WINDOWS = SSC_MAX_WINDOWS;
    #else
      static constexpr int MAX_WINDOWS = 32;
    #endif
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

  inline String getResolveToMainProcessMessage (
    const String& seq,
    const String& state,
    const String& value
  ) {
    return String("ipc://resolve?seq=" + seq + "&state=" + state + "&value=" + value);
  }

  struct CoreWindowOptions {
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
    MessageCallback onMessage = [](const String) {};
    ExitCallback onExit = nullptr;
    CoreDataManager* dataManager = nullptr;
  };

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

  class CoreWindow {
    public:
      CoreApplication& app;
      CoreWindowOptions opts;
      CoreWindowInternals *internals = nullptr;
      CoreWebView* webview = nullptr;
      CoreDataManager* dataManager = nullptr;

      MessageCallback onMessage = [](const String) {};
      ExitCallback onExit = nullptr;
      int index = 0;

      /*
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
*/
      CoreWindow () = delete;
      CoreWindow (
        CoreApplication&,
        const CoreWindowOptions,
        webview::CoreIPCSchemeRequestRouteCallback
      );

      void initialize ();
      void about ();
      void eval (const String&);
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
      /*
      #if defined(__linux__) && !defined(__ANDROID__)
      void closeContextMenu (GtkWidget *, const String&);
      #endif
      */
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

      void resolvePromise (
        const String& seq,
        const String& state,
        const String& value
      );
  };

  inline String createPreload (CoreWindowOptions opts) {
    static platform::PlatformInfo platform;
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
}
#endif
