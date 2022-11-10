#ifndef SSC_CORE_TYPES_HH
#if !defined(SSC_INLINE_INCLUDE)
#define SSC_CORE_TYPES_HH
#include "platform.hh"
#endif

#if !defined(SSC_INLINE_INCLUDE)
namespace ssc::types {
#endif

  using Any = std::any;
  using AtomicBool = std::atomic<bool>;
  using BinarySemaphore = std::binary_semaphore; // aka `Semaphore<1>`
  using ExitCallback = std::function<void(int code)>;
  using Map = std::map<std::string, std::string>;
  using Lock = std::lock_guard<std::recursive_mutex>;
  using MessageCallback = std::function<void(const std::string)>;
  using Mutex = std::recursive_mutex;

  template <int K> using Semaphore = std::counting_semaphore<K>;
  template <typename T> using SharedPointer = std::shared_ptr<T>;
  template <typename T> using UniquePointer = std::unique_ptr<T>;
  using String = std::string;
  using StringStream = std::stringstream;
  using Thread = std::thread;
  template <typename T> using Queue = std::queue<T>;
  template <typename T> using Vector = std::vector<T>;

  using WString = std::wstring;
  using WStringStream = std::wstringstream;

  struct Post {
    uint64_t id = 0;
    uint64_t ttl = 0;
    char *body = nullptr;
    size_t length = 0;
    String headers = "";
    bool bodyNeedsFree = false;
  };

  using Posts = std::map<uint64_t, Post>;

#if !defined(SSC_INLINE_INCLUDE)
}
#endif
#endif
