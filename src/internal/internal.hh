#ifndef SSC_INTERNAL_HH
#define SSC_INTERNAL_HH

#include <functional>
#include <string>

namespace ssc::internal {
  // opaque
  struct Bluetooth;
  struct Bridge;
  struct Platform;
  struct Router;

  template <class T, typename ...Args> T* make (Args... args) {
    T* pointer = nullptr;
    alloc(&pointer);
    init(pointer, args...);
    return pointer;
  }

  void alloc (Bluetooth**);
  void init (Bluetooth*);
  void deinit (Bluetooth*);

  void alloc (Bridge**);
  void init (Bridge*);
  void deinit (Bridge*);

  void alloc (Platform**);
  void init (Platform*);
  void deinit (Platform*);

  struct RouterOptions {
    std::function<void(std::string, std::string)> onNetworkStatusChange;
  };

  void alloc (Router**);
  void init (Router*, const RouterOptions& options);
  void deinit (Router*);

  namespace platform {
    using NotifyCallback = std::function<void(std::string)>;
    void notify (
      Platform*,
      const std::string& title,
      const std::string& body,
      NotifyCallback
    );

    using OpenExternalCallback = std::function<void(std::string)>;
    void openExternal (
      Platform*,
      const std::string& value,
      OpenExternalCallback
    );
  }

  namespace router {
    bool send (
      Router* router,
      const std::string& seq,
      const std::string& data,
      const char* bytes,
      size_t size
    );
  }
}
#endif
