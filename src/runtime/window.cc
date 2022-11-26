#include <socket/env.hh>
#include <socket/log.hh>
#include <socket/webview.hh>
#include <socket/window.hh>

namespace ssc::runtime::window {
  using namespace runtime;

  Window::Window (
    CoreApplication& app,
    Runtime& runtime,
    const WindowOptions opts
  )
    : app(app),
      runtime(runtime),
      opts(opts),
      bridge(runtime)
  {
    // preload script for normalizing the interface to be cross-platform.
    auto preloadScript = Script("preload.js", ToString(createPreload(opts)));
    this->internals = new WindowInternals(this, opts);
    this->webview = new WebView(
      this,
      opts.dataManager,
      [this](auto& request) {
        return this->onIPCSchemeRequestRouteCallback(request);
      },
      preloadScript
    );

    this->initialize();

    bridge.router.evaluateJavaScriptFunction = [&](auto script) {
      app.dispatch([=] {
        this->eval(script);
      });
    };
  }

  bool Window::onIPCSchemeRequestRouteCallback (
    webview::SchemeRequest& request
  ) {
    auto invoked = this->bridge.router.invoke(request.message, [=](auto result) mutable {
      JSON::Any json = result.json();
      auto data = result.data();
      auto seq = result.seq;

      // log::info("onIPCSchemeRequestRouteCallback: " + result.str());

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

  bool Window::onScriptMessage (const String& string) {
    auto message = ipc::Message(string);
    auto invoked = this->bridge.router.invoke(message);

    if (!invoked && this->onMessage != nullptr) {
      return this->onMessage(string);
    }

    return invoked;
  }

  void Window::resolvePromise (
    const String& seq,
    const String& state,
    const String& value
  ) {
    if (seq.find("R") == 0) {
      this->eval(getResolveToRenderProcessJavaScript(seq, state, value));
    }

    this->onMessage(getResolveToMainProcessMessage(seq, state, value));
  }

  void Window::exit (int code) {
    if (onExit != nullptr) onExit(code);
  }

  void Window::eval (Script script) {
    this->eval(script.str());
  }

  void Window::blur () {
    this->eval(Script("window.blur()"));
  }

  void Window::focus () {
    this->eval(Script("window.focus()"));
  }

  void Window::dispatchEvent (
    const String& name,
    const String& data
  ) {
    auto value = encodeURIComponent(data);
    auto script = getEmitToRenderProcessJavaScript(name, value);
    this->eval(script);
  }

  WindowManager::ManagedWindow::ManagedWindow (
    WindowManager& manager,
    CoreApplication& app,
    Runtime& runtime,
    WindowOptions opts
  ) : Window(app, runtime, opts),
      manager(manager)
  {}

  void WindowManager::ManagedWindow::show (const String &seq) {
    auto index = std::to_string(this->opts.index);
    manager.log("Showing Window#" + index + " (seq=" + seq + ")");
    status = WindowStatus::WINDOW_SHOWING;
    Window::show(seq);
    status = WindowStatus::WINDOW_SHOWN;
  }

  void WindowManager::ManagedWindow::hide (const String &seq) {
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

  void WindowManager::ManagedWindow::close (int code) {
    if (status < WindowStatus::WINDOW_CLOSING) {
      auto index = std::to_string(this->opts.index);
      manager.log("Closing Window#" + index + " (code=" + std::to_string(code) + ")");
      status = WindowStatus::WINDOW_CLOSING;
      Window::close(code);
      status = WindowStatus::WINDOW_CLOSED;
      // gc();
    }
  }

  void WindowManager::ManagedWindow::exit (int code) {
    if (status < WindowStatus::WINDOW_EXITING) {
      auto index = std::to_string(this->opts.index);
      manager.log("Exiting Window#" + index + " (code=" + std::to_string(code) + ")");
      status = WindowStatus::WINDOW_EXITING;
      Window::exit(code);
      status = WindowStatus::WINDOW_EXITED;
      gc();
    }
  }

  void WindowManager::ManagedWindow::kill () {
    if (status < WindowStatus::WINDOW_KILLING) {
      auto index = std::to_string(this->opts.index);
      manager.log("Killing Window#" + index);
      status = WindowStatus::WINDOW_KILLING;
      Window::kill();
      status = WindowStatus::WINDOW_KILLED;
      gc();
    }
  }

  void WindowManager::ManagedWindow::gc () {
    manager.destroyWindow(reinterpret_cast<Window*>(this));
  }

  WindowManager::WindowManager (
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

  WindowManager::~WindowManager () {
    destroy();
  }

  void WindowManager::destroy () {
    if (this->destroyed) return;
    for (auto window : windows) {
      destroyWindow(window);
    }

    this->destroyed = true;

    windows.clear();
    inits.clear();
  }

  void WindowManager::configure (WindowManagerOptions configuration) {
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

  void inline WindowManager::log (const String line) {
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

  Window* WindowManager::getWindow (int index, WindowStatus status) {
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

  Window* WindowManager::getWindow (int index) {
    return getWindow(index, WindowStatus::WINDOW_EXITING);
  }

  Window* WindowManager::getOrCreateWindow (int index) {
    return getOrCreateWindow(index, WindowOptions {});
  }

  Window* WindowManager::getOrCreateWindow (int index, WindowOptions opts) {
    if (this->destroyed) return nullptr;
    if (index < 0) return nullptr;
    if (getWindowStatus(index) == WindowStatus::WINDOW_NONE) {
      opts.index = index;
      return createWindow(opts);
    }

    return getWindow(index);
  }

  WindowManager::WindowStatus WindowManager::getWindowStatus (int index) {
    Lock lock(this->mutex);
    if (this->destroyed) return WindowStatus::WINDOW_NONE;
    if (index >= 0 && inits[index]) {
      return windows[index]->status;
    }

    return WindowStatus::WINDOW_NONE;
  }

  void WindowManager::destroyWindow (int index) {
    Lock lock(this->mutex);
    if (destroyed) return;
    if (index >= 0 && inits[index] && windows[index] != nullptr) {
      return destroyWindow(windows[index]);
    }
  }

  void WindowManager::destroyWindow (ManagedWindow* window) {
    if (destroyed) return;
    if (window != nullptr) {
      return destroyWindow(reinterpret_cast<Window*>(window));
    }
  }

  void WindowManager::destroyWindow (Window* window) {
    Lock lock(this->mutex);
    if (destroyed) return;
    if (window != nullptr && windows[window->index] != nullptr) {
      auto metadata = reinterpret_cast<ManagedWindow*>(window);

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

  Window* WindowManager::createWindow (WindowOptions opts) {
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

    WindowOptions windowOptions = {
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

    auto window = new ManagedWindow(*this, app, this->runtime, windowOptions);

    window->status = WindowStatus::WINDOW_CREATED;
    window->onExit = this->options.onExit;
    window->onMessage = this->options.onMessage;

    windows[opts.index] = window;
    inits[opts.index] = true;

    return reinterpret_cast<Window*>(window);
  }

  Window* WindowManager::createDefaultWindow (WindowOptions opts) {
    return createWindow(WindowOptions {
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
}
