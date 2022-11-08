#ifndef SSC_CORE_STRING_HH
#define SSC_CORE_STRING_HH

#include "types.hh"

#if !defined(SSC_INLINE_INCLUDE)
namespace ssc::string {
  using namespace ssc::types;
#endif

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
      if (n != c) {
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
      if(n != c) {
        buff += n;
      } else if (n == c && buff != "") {
        vec.push_back(buff);
        buff = "";
      }
    }

    if (!buff.empty()) vec.push_back(buff);

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

#if !defined(SSC_INLINE_INCLUDE)
}
#endif

#endif
