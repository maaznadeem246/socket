#ifndef SSC_CORE_WINDOW_HH
#define SSC_CORE_WINDOW_HH

#include <socket/platform.hh>
#include "application.hh"
#include "config.hh"
#include "data.hh"
#include "string.hh"
#include "javascript.hh"
#include "types.hh"
#include "utils.hh"
#include "webview.hh"

namespace ssc::core::window {
  using application::CoreApplication;
  using config::Config;
  using data::CoreDataManager;
  using javascript::Script;
  using string::String;
  using string::trim;
  using types::ExitCallback;
  using types::Map;
  using types::MessageCallback;
  using utils::encodeURIComponent;
  using webview::WebView;

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
    MessageCallback onMessage = [](const String) { return false; };
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

      MessageCallback onMessage = [](const String) { return false; };
      ExitCallback onExit = nullptr;
      int index = 0;

      CoreWindow () = delete;
      CoreWindow (
        CoreApplication&,
        const CoreWindowOptions,
        webview::CoreIPCSchemeRequestRouteCallback
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

      void resolvePromise (
        const String& seq,
        const String& state,
        const String& value
      );

      void dispatchEvent (
        const String& name,
        const String& data
      );

      void blur ();
      void focus ();

      virtual bool onScriptMessage (const String& string) = 0;
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

  class Window : public CoreWindow {
    public:
      Bridge bridge;
      Runtime& runtime;
      Window (CoreApplication& app, Runtime& runtime, const WindowOptions opts)
        : bridge(runtime),
          runtime(runtime),
          CoreWindow(app, opts, [this](auto& request) {
            return this->onIPCSchemeRequestRouteCallback(request);
          })
      {
        this->init();
      }

      inline void init () {
        bridge.router.evaluateJavaScriptFunction = [&](auto script) {
          app.dispatch([=] {
            this->eval(script);
          });
        };
      }

      bool onIPCSchemeRequestRouteCallback (
        webview::IPCSchemeRequest& request
      ) {
        auto invoked = this->bridge.router.invoke(request.message, [=](auto result) mutable {
          JSON::Any json = result.json();
          auto data = result.data();
          auto seq = result.seq;

          log::info("onIPCSchemeRequestRouteCallback: " + result.str());

          if (seq == "-1") {
            this->bridge.router.send(seq, json, data);
          } else {
            app.dispatch([=] () mutable {
              auto headers = data.headers;
              auto hasError = (
                json.isObject() &&
                json.as<JSON::Object>().has("err") &&
                json.as<JSON::Object>().get("err").isObject()
              );

              auto statusCode = hasError ? 500 : 200;
              log::info("here");
              log::info(json);
              request.end(statusCode, headers, json, data.body, data.length);
            });
          }
        });

        if (!invoked && this->onMessage != nullptr) {
          invoked = this->onMessage(request.message.str());

          if (invoked) {
            auto seq = request.message.seq;
            auto json = JSON::Object::Entries {
              {"source", request.message.name},
              {"data", JSON::Object::Entries {
                {"seq", seq.size() > 0 ? JSON::Any(seq) : JSON::null}
              }}
            };

            request.end(200, Headers {}, json, nullptr, 0);
            return true;
          }
        }

        if (!invoked) {
          auto json = JSON::Object::Entries {
            {"source", request.message.name},
            {"err", JSON::Object::Entries {
              {"message", "Not found"},
              {"type", "NotFoundError"},
              {"url", request.url}
            }}
          };

          request.end(404, Headers {}, json, nullptr, 0);
        }

        return true; // request handled
      }

      bool onScriptMessage (const String& string) {
        auto message = ipc::Message(string);
        auto invoked = this->bridge.router.invoke(message);

        if (!invoked && this->onMessage != nullptr) {
          return this->onMessage(string);
        }

        return invoked;
      }
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

      class WindowWithMetadata : public Window {
        public:
          WindowStatus status;
          WindowManager &manager;

          WindowWithMetadata (
            WindowManager& manager,
            CoreApplication& app,
            Runtime& runtime,
            CoreWindowOptions opts
          ) : Window(app, runtime, opts) , manager(manager) { }

          ~WindowWithMetadata () {}

          void show (const String &seq) {
            auto index = std::to_string(this->opts.index);
            manager.log("Showing Window#" + index + " (seq=" + seq + ")");
            status = WindowStatus::WINDOW_SHOWING;
            Window::show(seq);
            status = WindowStatus::WINDOW_SHOWN;
          }

          void hide (const String &seq) {
            if (
              status > WindowStatus::WINDOW_HIDDEN &&
              status < WindowStatus::WINDOW_EXITING
            ) {
              auto index = std::to_string(this->opts.index);
              manager.log("Hiding Window#" + index + " (seq=" + seq + ")");
              status = WindowStatus::WINDOW_HIDING;
              Window::hide(seq);
              status = WindowStatus::WINDOW_HIDDEN;
            }
          }

          void close (int code) {
            if (status < WindowStatus::WINDOW_CLOSING) {
              auto index = std::to_string(this->opts.index);
              manager.log("Closing Window#" + index + " (code=" + std::to_string(code) + ")");
              status = WindowStatus::WINDOW_CLOSING;
              Window::close(code);
              status = WindowStatus::WINDOW_CLOSED;
              // gc();
            }
          }

          void exit (int code) {
            if (status < WindowStatus::WINDOW_EXITING) {
              auto index = std::to_string(this->opts.index);
              manager.log("Exiting Window#" + index + " (code=" + std::to_string(code) + ")");
              status = WindowStatus::WINDOW_EXITING;
              Window::exit(code);
              status = WindowStatus::WINDOW_EXITED;
              gc();
            }
          }

          void kill () {
            if (status < WindowStatus::WINDOW_KILLING) {
              auto index = std::to_string(this->opts.index);
              manager.log("Killing Window#" + index);
              status = WindowStatus::WINDOW_KILLING;
              Window::kill();
              status = WindowStatus::WINDOW_KILLED;
              gc();
            }
          }

          void gc () {
            manager.destroyWindow(reinterpret_cast<Window*>(this));
          }
      };

      std::chrono::system_clock::time_point lastDebugLogLine;

      CoreApplication &app;
      bool destroyed = false;
      std::vector<bool> inits;
      std::vector<WindowWithMetadata*> windows;
      std::recursive_mutex mutex;
      WindowManagerOptions options;
      Runtime& runtime;

      WindowManager (
        CoreApplication& app,
        Runtime& runtime
      ) :
        app(app),
        runtime(runtime),
        inits(MAX_WINDOWS),
        windows(MAX_WINDOWS)
    {
      this->options.dataManager = &this->runtime.dataManager;
      if (ssc::config::isDebugEnabled()) {
        lastDebugLogLine = std::chrono::system_clock::now();
      }
    }

      ~WindowManager () {
        destroy();
      }

      void destroy () {
        if (this->destroyed) return;
        for (auto window : windows) {
          destroyWindow(window);
        }

        this->destroyed = true;

        windows.clear();
        inits.clear();
      }

      void configure (WindowManagerOptions configuration) {
        if (destroyed) return;
        this->options.defaultHeight = configuration.defaultHeight;
        this->options.defaultWidth = configuration.defaultWidth;
        this->options.isHeightInPercent = configuration.isHeightInPercent;
        this->options.isWidthInPercent = configuration.isWidthInPercent;
        this->options.onMessage = configuration.onMessage;
        this->options.config = configuration.config;
        this->options.onExit = configuration.onExit;
        this->options.headless = configuration.headless;
        this->options.isTest = configuration.isTest;
        this->options.argv = configuration.argv;
        this->options.cwd = configuration.cwd;
        if (configuration.dataManager != nullptr) {
          this->options.dataManager = configuration.dataManager;
        }
      }

      void inline log (const String line) {
        if (!ssc::config::isDebugEnabled() || destroyed) return;
        using namespace std::chrono;
        #ifdef _WIN32 // unicode console support
          // SetConsoleOutputCP(CP_UTF8);
          // setvbuf(stdout, nullptr, _IOFBF, 1000);
        #endif

        auto now = system_clock::now();
        auto delta = duration_cast<milliseconds>(now - lastDebugLogLine).count();
        StringStream stream;

        stream << "• " << line;
        stream << " \033[0;32m+";
        stream << delta;
        stream << "ms\033[0m";

        log::info(stream);

        lastDebugLogLine = now;
      }

      Window* getWindow (int index, WindowStatus status) {
        std::lock_guard<std::recursive_mutex> guard(this->mutex);
        if (this->destroyed) return nullptr;
        if (
          getWindowStatus(index) > WindowStatus::WINDOW_NONE &&
          getWindowStatus(index) < status
        ) {
          return reinterpret_cast<Window*>(windows[index]);
        }

        return nullptr;
      }

      Window* getWindow (int index) {
        return getWindow(index, WindowStatus::WINDOW_EXITING);
      }

      Window* getOrCreateWindow (int index) {
        return getOrCreateWindow(index, CoreWindowOptions {});
      }

      Window* getOrCreateWindow (int index, CoreWindowOptions opts) {
        if (this->destroyed) return nullptr;
        if (index < 0) return nullptr;
        if (getWindowStatus(index) == WindowStatus::WINDOW_NONE) {
          opts.index = index;
          return createWindow(opts);
        }

        return getWindow(index);
      }

      WindowStatus getWindowStatus (int index) {
        std::lock_guard<std::recursive_mutex> guard(this->mutex);
        if (this->destroyed) return WindowStatus::WINDOW_NONE;
        if (index >= 0 && inits[index]) {
          return windows[index]->status;
        }

        return WindowStatus::WINDOW_NONE;
      }

      void destroyWindow (int index) {
        std::lock_guard<std::recursive_mutex> guard(this->mutex);
        if (destroyed) return;
        if (index >= 0 && inits[index] && windows[index] != nullptr) {
          return destroyWindow(windows[index]);
        }
      }

      void destroyWindow (WindowWithMetadata* window) {
        if (destroyed) return;
        if (window != nullptr) {
          return destroyWindow(reinterpret_cast<Window*>(window));
        }
      }

      void destroyWindow (Window* window) {
        std::lock_guard<std::recursive_mutex> guard(this->mutex);
        if (destroyed) return;
        if (window != nullptr && windows[window->index] != nullptr) {
          auto metadata = reinterpret_cast<WindowWithMetadata*>(window);

          inits[window->index] = false;
          windows[window->index] = nullptr;

          if (metadata->status < WINDOW_CLOSING) {
            window->close(0);
          }

          if (metadata->status < WINDOW_KILLING) {
            window->kill();
          }

          delete window;
        }
      }

      Window* createWindow (CoreWindowOptions opts) {
        std::lock_guard<std::recursive_mutex> guard(this->mutex);
        if (destroyed) return nullptr;
        StringStream env;

        if (inits[opts.index]) {
          return reinterpret_cast<Window*>(windows[opts.index]);
        }

        for (auto const &envKey : split(opts.config["env"], ',')) {
          auto cleanKey = trim(envKey);
          auto envValue = env::get(cleanKey);

          env << String(
            cleanKey + "=" + encodeURIComponent(envValue) + "&"
          );
        }

        for (auto const &envKey : split(this->options.config["env"], ',')) {
          auto cleanKey = trim(envKey);
          auto envValue = env::get(cleanKey);

          env << String(
            cleanKey + "=" + encodeURIComponent(envValue) + "&"
          );
        }

        auto height = opts.height > 0 ? opts.height : this->options.defaultHeight;
        auto width = opts.width > 0 ? opts.width : this->options.defaultWidth;
        auto isHeightInPercent = opts.height > 0 ? false : this->options.isHeightInPercent;
        auto isWidthInPercent = opts.width > 0 ? false : this->options.isWidthInPercent;

        CoreWindowOptions windowOptions = {
          .resizable = opts.resizable,
          .frameless = opts.frameless,
          .utility = opts.utility,
          .canExit = opts.canExit,
          .height = height,
          .width = width,
          .isHeightInPercent = isHeightInPercent,
          .isWidthInPercent = isWidthInPercent,
          .index = opts.index,
          .debug = ssc::config::isDebugEnabled() || opts.debug,
          .port = opts.port,
          .isTest = this->options.isTest,
          .headless = this->options.headless || opts.headless || opts.config["headless"] == "true",
          .forwardConsole = opts.config["linux_forward_console_to_stdout"] == "true",

          .cwd = opts.cwd.size() > 0 ? opts.cwd
            : opts.config.has("cwd") ? opts.config.get("cwd")
              : this->options.cwd.size() > 0 ? this->options.cwd
                : app.cwd(),

          .executable = opts.executable.size() > 0  ? opts.executable : app.config.get("executable"),
          .title = opts.title.size() > 0 ? opts.title : opts.config["title"],
          .url = opts.url.size() > 0 ? opts.url : "data:text/html,<html>",
          .version = "v" + opts.config["version"],
          .argv = this->options.argv,
          .preload = opts.preload.size() > 0 ? opts.preload : "",
          .env = env.str(),
          .config = opts.config.size() > 0 ? opts.config : this->options.config,
          .dataManager = opts.dataManager != nullptr ? opts.dataManager : this->options.dataManager
        };

        if (ssc::config::isDebugEnabled()) {
          this->log("Creating Window#" + std::to_string(opts.index));
        }

        auto window = new WindowWithMetadata(*this, app, this->runtime, windowOptions);

        window->status = WindowStatus::WINDOW_CREATED;
        window->onExit = this->options.onExit;
        window->onMessage = this->options.onMessage;

        windows[opts.index] = window;
        inits[opts.index] = true;

        return reinterpret_cast<Window*>(window);
      }

      Window* createDefaultWindow (CoreWindowOptions opts) {
        return createWindow(CoreWindowOptions {
          .resizable = true,
          .frameless = false,
          .canExit = true,
          .height = opts.height,
          .width = opts.width,
          .index = 0,
          .port = ssc::config::getServerPort(),
          .config = opts.config,
          .dataManager = opts.dataManager
        });
      }
  };
}
#endif
