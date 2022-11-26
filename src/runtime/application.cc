#include <socket/socket.hh>
#include <uv.h>

#if defined(_WIN32)
#include <uxtheme.h>
#pragma comment(lib, "UxTheme.lib")
#pragma comment(lib, "Dwmapi.lib")
#pragma comment(lib, "Gdi32.lib")
#endif

#include "application.hh"
#include "window.hh"

using namespace ssc;

static const String OK_STATE = "0";
static const String ERROR_STATE = "1";
static const String EMPTY_SEQ = String("");

#if defined(__APPLE__)
static dispatch_queue_attr_t qos = dispatch_queue_attr_make_with_qos_class(
  DISPATCH_QUEUE_CONCURRENT,
  QOS_CLASS_USER_INITIATED,
  -1
);

static dispatch_queue_t queue = dispatch_queue_create(
  "co.socketsupply.queue.app",
  qos
);
#endif

#if defined(_WIN32)
static inline void alert (const SSC::WString& ws) {
  auto s = WStringToString(ws);
  MessageBoxA(nullptr, s.c_str(), _TEXT("Alert"), MB_OK | MB_ICONSTOP);
}

static inline void alert (const SSC::String& s) {
  MessageBoxA(nullptr, s.c_str(), _TEXT("Alert"), MB_OK | MB_ICONSTOP);
}

static inline void alert (const char* s) {
  MessageBoxA(nullptr, s, _TEXT("Alert"), MB_OK | MB_ICONSTOP);
}
#endif

namespace ssc::runtime::application {
  Application::Application (
    const int argc,
    const char** argv
  ) : argc(argc), argv(argv) {
    this->wasStartedFromCli = env::has("SSC_CLI");
    ssc::init(this->config, argc, argv);
    Application::instance = this;
  }

  Application::~Application () {
    Application::instance = nullptr;
  }

#if !defined(_WIN32)
  Application::Application (
    int unused,
    const int argc,
    const char** argv
  ) : Application(argc, argv) {
    // noop
  }
#else
  Application::Application (
    void *hInstance,
    const int argc,
    const char** argv
  ) : Application() {
    this->hInstance = hInstance; // HINSTANCE

    HMODULE hUxtheme = LoadLibraryExW(L"uxtheme.dll", nullptr, LOAD_LIBRARY_SEARCH_SYSTEM32);

    ssc::runtime::global:::setWindowCompositionAttribute = reinterpret_cast<ssc::runtime::global::SetWindowCompositionAttribute>(GetProcAddress(
      GetModuleHandleW(L"user32.dll"),
      "SetWindowCompositionAttribute")
    );

    if (hUxtheme) {
      ssc::runtime::global::refreshImmersiveColorPolicyState = GetProcAddress(hUxtheme, MAKEINTRESOURCEA(104));
      ssc::runtime::global::shouldSystemUseDarkMode = GetProcAddress(hUxtheme, MAKEINTRESOURCEA(138));
      ssc::runtime::global::allowDarkModeForApp = GetProcAddress(hUxtheme, MAKEINTRESOURCEA(135));
    }

    allowDarkModeForApp(shouldSystemUseDarkMode());
    refreshImmersiveColorPolicyState();

    // this fixes bad default quality DPI.
    SetProcessDPIAware();

    auto iconPath = fs::path { cwd() / fs::path { "index.ico" } };

    auto icon = (HICON) LoadImageA(
      NULL,
      iconPath.string().c_str(),
      IMAGE_ICON,
      GetSystemMetrics(SM_CXSMICON),
      GetSystemMetrics(SM_CXSMICON),
      LR_LOADFROMFILE
    );

    auto szWindowClass = L"DesktopApp";
    auto szTitle = L"Socket";

    wcex.cbSize = sizeof(WNDCLASSEX);
    wcex.style = CS_HREDRAW | CS_VREDRAW;
    wcex.cbClsExtra = 0;
    wcex.cbWndExtra = 0;
    wcex.hInstance = hInstance;
    wcex.hIcon = LoadIcon(hInstance, IDI_APPLICATION);
    wcex.hCursor = LoadCursor(NULL, IDC_ARROW);
    wcex.hbrBackground = bgBrush;
    wcex.lpszMenuName = NULL;
    wcex.lpszClassName = TEXT("DesktopApp");
    wcex.hIconSm = icon; // ico doesn't auto scale, needs 16x16 icon lol fuck you bill
    wcex.hIcon = icon;
    //wcex.lpfnWndProc = Window::WndProc; // FIXME

    if (!RegisterClassEx(&wcex)) {
      alert("Application could not launch, possible missing resources.");
    }
  }
#endif

  int Application::run () {
  #if defined(__linux__)
    gtk_main();
  #elif defined(__APPLE__) && !TARGET_OS_IPHONE && !TARGET_IPHONE_SIMULATOR
    [NSApp run];
  #elif defined(_WIN32)
    MSG msg;

    if (!GetMessage(&msg, nullptr, 0, 0)) {
      exitWasRequested = true;
      return 1;
    }

    if (msg.hwnd) {
      TranslateMessage(&msg);
      DispatchMessage(&msg);
    }

    if (msg.message == WM_APP) {
      // from PostThreadMessage
      auto callback = (std::function<void()> *)(msg.lParam);
      (*callback)();
      delete callback;
    }

    if (msg.message == WM_QUIT && shouldExit) {
      return 1;
    }
  #endif

    return exitWasRequested ? 1 : 0;
  }

  void Application::kill () {
    // Distinguish window closing with app exiting
    exitWasRequested = true;
  #if defined(__linux__)
    gtk_main_quit();
  #elif defined(__APPLE__) && !TARGET_OS_IPHONE && !TARGET_IPHONE_SIMULATOR
    // if not launched from the cli, just use `terminate()`
    // exit code status will not be captured
    if (!wasStartedFromCli) {
      [NSApp terminate:nil];
    }
  #elif defined(_WIN32)
    PostQuitMessage(0);
  #endif
  }

  void Application::restart () {
  #if defined(__linux__)
    // @TODO
  #elif defined(__APPLE__)
    // @TODO
  #elif defined(_WIN32)
    char filename[MAX_PATH] = "";
    PROCESS_INFORMATION pi;
    STARTUPINFO si = { sizeof(STARTUPINFO) };
    GetModuleFileName(NULL, filename, MAX_PATH);
    CreateProcess(NULL, filename, NULL, NULL, NULL, NULL, NULL, NULL, &si, &pi);
    std::exit(0);
  #endif
  }

  void Application::dispatch (std::function<void()> callback) {
  #if defined(__linux__)
    auto threadCallback = new std::function<void()>(callback);

    g_idle_add_full(
      G_PRIORITY_HIGH_IDLE,
      (GSourceFunc)([](void* callback) -> int {
        (*static_cast<std::function<void()>*>(callback))();
        return G_SOURCE_REMOVE;
      }),
      threadCallback,
      [](void* callback) {
        delete static_cast<std::function<void()>*>(callback);
      }
    );
  #elif defined(__APPLE__)
    auto priority = DISPATCH_QUEUE_PRIORITY_DEFAULT;
    auto queue = dispatch_get_global_queue(priority, 0);

    dispatch_async(queue, ^{
      dispatch_sync(dispatch_get_main_queue(), ^{
        callback();
      });
    });
  #elif defined(_WIN32)
    static auto mainThread = GetCurrentThreadId();
    auto future = std::async(std::launch::async, [&] {
      auto threadCallback = (LPARAM) new std::function<void()>(callback);

      while (!this->isReady) {
        std::this_thread::sleep_for(std::chrono::milliseconds(16));
      }

      PostThreadMessage(mainThread, WM_APP, 0, threadCallback);
    });

    future.get();
  #endif
  }

  String Application::cwd () {
    String cwd = "";

    #if defined(__linux__) && !defined(__ANDROID__)
      auto canonical = fs::canonical("/proc/self/exe");
      cwd = fs::path(canonical).parent_path().string();
    #elif defined(__ANDROID__)
      // TODO
    #elif defined(__APPLE__)
      #if TARGET_OS_IPHONE || TARGET_IPHONE_SIMULATOR
        NSFileManager *fileManager = [NSFileManager defaultManager];
        NSString *currentDirectoryPath = [fileManager currentDirectoryPath];
        cwd = [[NSHomeDirectory() stringByAppendingPathComponent: currentDirectoryPath] UTF8String];
      #else
        NSString *bundlePath = [[NSBundle mainBundle] resourcePath];
        cwd = [bundlePath UTF8String];
      #endif
    #elif defined(_WIN32)
      wchar_t filename[MAX_PATH];
      GetModuleFileNameW(NULL, filename, MAX_PATH);
      auto path = fs::path { filename }.remove_filename();
      cwd = path.string();
    #endif

    return cwd;
  }

  void Application::exit (int code) {
    if (this->callbacks.onExit != nullptr) {
      this->callbacks.onExit(code);
    }
  }
}

#if defined(__APPLE__) && (TARGET_OS_IPHONE || TARGET_IPHONE_SIMULATOR)
@implementation IOSApplication
//
// iOS has no "window". There is no navigation, just a single webview. It also
// has no "main" process, we want to expose some network functionality to the
// JavaScript environment so it can be used by the web app and the wasm layer.
//
- (void) applicationDidEnterBackground: (UIApplication*) application {
  window->blur();
}

- (void) applicationWillEnterForeground: (UIApplication*) application {
  window->focus();
}

- (void) applicationWillTerminate: (UIApplication*) application {
  // @TODO(jwerle): what should we do here?
}

- (void) applicationDidBecomeActive: (UIApplication*) application {
  dispatch_async(queue, ^{
    app->onResume();
    app->start();
  });
}

- (void) applicationWillResignActive: (UIApplication*) application {
  dispatch_async(queue, ^{
    app->onPause();
    app->stop();
  });
}

-            (BOOL) application: (UIApplication*) application
  didFinishLaunchingWithOptions: (NSDictionary*) launchOptions
{

  app = ssc::runtime::application::Application::getInstance();
  window = app->createDefaultWindow();

  return YES;
}
@end
#endif
