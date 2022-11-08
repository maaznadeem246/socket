#ifndef SSC_INTERNAL_HH
#define SSC_INTERNAL_HH

#include <functional>
#include <map>
#include <mutex>
#include <string>
#include <vector>

namespace ssc::internal {
  using Lock = std::lock_guard<std::recursive_mutex>;
  using Mutex = std::recursive_mutex;
  using String = std::string;
  using Seq = std::string;

  template <typename T> using Vector = std::vector<T>;

  // forward opaque
  struct Bluetooth;
  struct Bridge;
  struct Platform;
  struct Router;

  namespace bluetooth {}
  namespace bridge {}

  namespace platform {
    using NotifyCallback = std::function<void(String)>;
    using OpenExternalCallback = std::function<void(String)>;
  }

  namespace router {
    struct Options;
    using NetworkStatusChangeCallback = std::function<void(String, String)>;
    using InvokeResultCallback = std::function<void(Seq, String, char *, size_t)>;
    using InvokeCallback = std::function<bool(String, InvokeResultCallback)>;
  }

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

  void alloc (Router**);
  void init (Router*, const router::Options& options);
  void deinit (Router*);

  namespace platform {
    void notify (
      Platform*,
      const std::string& title,
      const std::string& body,
      NotifyCallback
    );

    void openExternal (
      Platform*,
      const std::string& value,
      OpenExternalCallback
    );
  }

  namespace router {
    struct Options {
      NetworkStatusChangeCallback onNetworkStatusChange;
      InvokeCallback onInvoke;
    };

    bool send (
      Router* router,
      const std::string& seq,
      const std::string& data,
      const char* bytes,
      size_t size
    );

    bool invoke (
      Router* router,
      String uri,
      InvokeResultCallback cb
    );
  }
}
#endif
