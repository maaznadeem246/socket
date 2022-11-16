#ifndef SSC_CORE_TYPES_HH
#define SSC_CORE_TYPES_HH

#include <socket/platform.hh>

#include <any>
#include <map>
#include <mutex>
#include <new>
#include <queue>
#include <regex>
#include <semaphore>
#include <string>
#include <sstream>
#include <thread>
#include <vector>

namespace ssc::core::types {
  using Any = std::any;
  using AtomicBool = std::atomic<bool>;
  using BinarySemaphore = std::binary_semaphore; // aka `Semaphore<1>`
  using ExitCallback = std::function<void(int code)>;
  using Map = std::map<std::string, std::string>;
  using Lock = std::lock_guard<std::recursive_mutex>;
  using MessageCallback = std::function<bool(const std::string)>;
  using Mutex = std::recursive_mutex;

  using String = std::string;
  using StringStream = std::stringstream;
  using Thread = std::thread;
  using WString = std::wstring;
  using WStringStream = std::wstringstream;

  template <int K> using Semaphore = std::counting_semaphore<K>;
  template <typename T> using SharedPointer = std::shared_ptr<T>;
  template <typename T> using UniquePointer = std::unique_ptr<T>;
  template <typename T> using Function = std::function<T>;
  template <typename T> using Queue = std::queue<T>;
  template <typename T> using Vector = std::vector<T>;

  // FIXME: remove this eventuallyk
  struct Post {
    uint64_t id = 0;
    uint64_t ttl = 0;
    char *body = nullptr;
    size_t length = 0;
    String headers = "";
    bool bodyNeedsFree = false;
  };

  using Posts = std::map<uint64_t, Post>;
}
#endif
