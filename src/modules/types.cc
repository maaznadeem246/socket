module;

#include <map>
#include <mutex>
#include <queue>
#include <semaphore>
#include <sstream>
#include <string>
#include <vector>

/**
 * @module ssc.types
 * @description Exports various types commonly used in ssc modules.
 */
export module ssc.types;
export namespace ssc {
  using AtomicBool = std::atomic<bool>;
  using BinarySemaphore = std::binary_semaphore; // aka `Semaphore<1>`
  using Map = std::map<std::string, std::string>;
  using Lock = std::lock_guard<std::recursive_mutex>;
  using Mutex = std::recursive_mutex;
  template <int K> using Semaphore = std::counting_semaphore<K>;
  template <typename T> using SharedPointer = std::shared_ptr<T>;
  using String = std::string;
  using StringStream = std::stringstream;
  using Thread = std::thread;
  template <typename T> using Queue = std::queue<T>;
  template <typename T> using Vector = std::vector<T>;

  using WString = std::wstring;
  using WStringStream = std::wstringstream;

  inline WString ToWString (String& string) {
    return WString(string.begin(), string.end());
  }

  inline WString ToWString (WString& string) {
    return string;
  }

  inline String ToString (WString& string) {
    return String(string.begin(), string.end());
  }

  inline String ToString (String& string) {
    return string;
  }
}
