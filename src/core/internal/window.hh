#ifndef SSC_CORE_INTERNAL_WINDOW_HH
#define SSC_CORE_INTERNAL_WINDOW_HH

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
#endif
#endif

namespace ssc::core::window {
  class CoreWindowInternals {
    public:
    #if defined(__APPLE__)
      #if !TARGET_OS_IPHONE && !TARGET_IPHONE_SIMULATOR
        NSWindow* window = nullptr;
        CoreWindowDelegate* delegate = nullptr;
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

      CoreWindowInternals (
        CoreWindow* coreWindow,
        const CoreWindowOptions& opts
      );

    #if defined(__linux__) && !defined(__ANDROID__)
      void closeContextMenu (GtkWidget *, const String&);
    #endif
  };
}

#endif
