#ifndef SSC_RUNTIME_GLOBAL_HH
#define SSC_RUNTIME_GLOBAL_HH

#include <socket/platform.hh>
#include <socket/common.hh>

#include "window.hh"

namespace ssc::runtime::global {
#if defined(_WIN32)
  using RefreshImmersiveColorPolicyState = VOID(WINAPI*)();
  using SetWindowCompositionAttribute = BOOL(WINAPI *)(HWND hWnd, WINDOWCOMPOSITIONATTRIBDATA*);
  using ShouldSystemUseDarkMode = BOOL(WINAPI*)();
  using AllowDarkModeForApp = BOOL(WINAPI*)(BOOL allow);

  extern RefreshImmersiveColorPolicyState refreshImmersiveColorPolicyState;
  extern SetWindowCompositionAttribute setWindowCompositionAttribute;
  extern ShouldSystemUseDarkMode shouldSystemUseDarkMode;
  extern AllowDarkModeForApp allowDarkModeForApp;

  static auto bgBrush = CreateSolidBrush(RGB(0, 0, 0));
#endif
}

#endif
