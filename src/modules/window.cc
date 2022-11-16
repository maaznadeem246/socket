module;
#include "../core/application.hh"
#include "../core/window.hh"

/**
 * @module window
 * @description Platform agnostic Window APIs
 */
export module ssc.window;
import ssc.bridge;
import ssc.config;
import ssc.data;
import ssc.env;
import ssc.headers;
import ssc.ipc;
import ssc.json;
import ssc.log;
import ssc.runtime;
import ssc.string;
import ssc.types;
import ssc.webview;
import ssc.utils;

using namespace ssc::types;
using namespace ssc::utils;
using namespace ssc::string;

using ssc::bridge::Bridge;
using ssc::config::Config;
using ssc::data::DataManager;
using ssc::headers::Headers;
using ssc::runtime::Runtime;

using ssc::core::application::CoreApplication;

export namespace ssc::window {
  using core::window::CoreWindow; // NOLINT
  using core::window::CoreWindowOptions; // NOLINT

  using WindowOptions = ssc::core::window::CoreWindowOptions;

  constexpr auto& MAX_WINDOWS = ssc::core::window::MAX_WINDOWS;

  class Window : public CoreWindow {
    public:
      Bridge bridge;
      Window (CoreApplication& app, Runtime& runtime, const WindowOptions opts)
        : bridge(runtime),
          CoreWindow(app, opts, [this](auto& request) {
            return this->onIPCSchemeRequestRouteCallback(request);
          })
      {
        this->init();
      }

      inline void init () {
        bridge.router.evaluateJavaScriptFunction = [&](auto script) {
          this->eval(script);
        };
      }

      bool onIPCSchemeRequestRouteCallback (
        webview::IPCSchemeRequest& request
      ) {
        auto invoked = this->bridge.router.invoke(request.message, [=](auto result) mutable {
          JSON::Any json = result.json();
          auto data = result.data();
          auto seq = result.seq;

          if (seq == "-1") {
            this->bridge.router.send(seq, json, data);
          } else {
            auto headers = data.headers;
            auto hasError = (
              json.isObject() &&
              json.as<JSON::Object>().has("err") &&
              json.as<JSON::Object>().get("err").isObject()
            );

            auto statusCode = hasError ? 500 : 200;

            request.end(statusCode, headers, json, data.body, data.length);
          }
        });

        if (!invoked && this->onMessage != nullptr) {
          invoked = this->onMessage(request.message.str());

          if (invoked) {
            auto json = JSON::Object::Entries {
              {"source", request.message.name},
              {"data", JSON::Object::Entries { }}
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

          .cwd = this->options.cwd,
          .executable = opts.config["executable"],
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
