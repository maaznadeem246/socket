#ifndef SSC_CORE_PLATFORM_HH
#define SSC_CORE_PLATFORM_HH

#include "string.hh"
#include "types.hh"

namespace ssc::core::platform {
  using namespace core::string;

  using NotifyCallback = Function<void(String)>;
  using OpenExternalCallback = Function<void(String)>;
  using CWDCallback = Function<void(String)>;

  class CorePlatform {
    public:
      void notify (
        const String& title,
        const String& body,
        NotifyCallback callback
      ) const;

      void openExternal (
        const String& value,
        OpenExternalCallback callback
      ) const;

      void cwd (CWDCallback calback) const;
  };
}
#endif
