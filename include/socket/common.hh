#ifndef SSC_SOCKET_COMMON_H
#define SSC_SOCKET_COMMON_H

#define _STR_VALUE(arg) #arg
#define STR_VALUE(arg) _STR_VALUE(arg)

#ifndef DEBUG
#define DEBUG 0
#endif

#ifndef SSC_CONFIG
#define SSC_CONFIG ""
#endif

#ifndef SSC_VERSION
#define SSC_VERSION ""
#endif

#ifndef SSC_VERSION_HASH
#define SSC_VERSION_HASH ""
#endif

#if !DEBUG
  #if defined(debug)
    #undef debug
  #endif
  #define debug(format, ...)
#endif

// macOS/iOS
#if defined(__APPLE__)
  #if !defined(debug)
    #define debug(format, ...) fprintf(stderr, format "\n", ##__VA_ARGS__)
  #endif
#endif

// Linux
#if defined(__linux__) && !defined(__ANDROID__)
  #if !defined(debug)
    #define debug(format, ...) fprintf(stderr, format "\n", ##__VA_ARGS__)
  #endif
#endif

// Android (Linux)
#if defined(__linux__) && defined(__ANDROID__)
  #if !defined(debug)
    #define debug(format, ...) \
      __android_log_print(     \
          ANDROID_LOG_DEBUG,   \
          __FUNCTION__,        \
          format,              \
          ##__VA_ARGS__        \
      );
  #endif
#endif

// Windows
#if defined(_WIN32)
  #define isatty _isatty
  #define fileno _fileno
  #if !defined(debug)
    #define debug(format, ...) fprintf(stderr, format "\n", ##__VA_ARGS__)
  #endif
#endif

#include <any>
#include <exception>
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

namespace ssc {
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

  inline WString ToWString (const String& string) {
    return WString(string.begin(), string.end());
  }

  inline WString ToWString (const WString& string) {
    return string;
  }

  inline String ToString (const WString& string) {
    return String(string.begin(), string.end());
  }

  inline String ToString (const String& string) {
    return string;
  }

  template <typename T> auto toString (T value) {
    return std::to_string(value);
  }

  inline WString StringToWString (const String& string) {
    return ToWString(string);
  }

  inline WString StringToWString (const WString& string) {
    return string;
  }

  inline String WStringToString (const WString& string) {
    return ToString(string);
  }

  inline String WStringToString (const String& s) {
    return s;
  }

  inline const Vector<String> splitc (const String& s, const char c) {
    String buff;
    Vector<String> vec;

    for (auto n : s) {
      if (c == 0) {
        buff += n;
        vec.push_back(buff);
        buff = "";
      } else if (n != c) {
        buff += n;
      } else if (n == c) {
        vec.push_back(buff);
        buff = "";
      }
    }

    vec.push_back(buff);
    return vec;
  }

  template <typename ...Args> String format (const String& s, Args ...args) {
    auto copy = s;
    StringStream res;
    Vector<Any> vec;
    using unpack = int[];

    (void) unpack { 0, (vec.push_back(args), 0)... };

    std::regex re("\\$[^$\\s]");
    std::smatch match;
    auto first = std::regex_constants::format_first_only;
    int index = 0;

    while (std::regex_search(copy, match, re) != 0) {
      if (match.str() == "$S") {
        auto value = std::any_cast<String>(vec[index++]);
        copy = std::regex_replace(copy, re, value, first);
      } else if (match.str() == "$i") {
        auto value = std::any_cast<int>(vec[index++]);
        copy = std::regex_replace(copy, re, std::to_string(value), first);
      } else if (match.str() == "$C") {
        auto value = std::any_cast<char*>(vec[index++]);
        copy = std::regex_replace(copy, re, String(value), first);
      } else if (match.str() == "$c") {
        auto value = std::any_cast<char>(vec[index++]);
        copy = std::regex_replace(copy, re, String(1, value), first);
      } else {
        copy = std::regex_replace(copy, re, match.str(), first);
      }
    }

    return copy;
  }

  inline String replace (const String& src, const String& re, const String& val) {
    return std::regex_replace(src, std::regex(re), val);
  }

  inline String& replaceAll (String& src, String const& from, String const& to) {
    size_t start = 0;
    size_t index;
    while ((index = src.find(from, start)) != String::npos) {
      src.replace(index, from.size(), to);
      start = index + to.size();
    }
    return src;
  }

  inline const Vector<String> split (const String& s, const char c) {
    String buff;
    Vector<String> vec;

    for (auto n : s) {
      if (c == 0) {
        buff += n;
        vec.push_back(buff);
        buff = "";
      } else if (n != c) {
        buff += n;
      } else if (n ==  c && buff != "") {
        vec.push_back(buff);
        buff = "";
      }
    }

    if (!buff.empty()) {
      vec.push_back(buff);
    }

    return vec;
  }

  inline String trim (String str) {
    str.erase(0, str.find_first_not_of(" \r\n\t"));
    str.erase(str.find_last_not_of(" \r\n\t") + 1);
    return str;
  }

  inline String tmpl (const String s, Map pairs) {
    String output = s;
    for (auto item : pairs) {
      auto key = String("[{]+(" + item.first + ")[}]+");
      auto value = item.second;
      output = std::regex_replace(output, std::regex(key), value);
    }
    return output;
  }
}

#endif
