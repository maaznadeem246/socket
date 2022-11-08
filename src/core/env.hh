#ifndef SSC_CORE_ENV_HH
#define SSC_CORE_ENV_HH

#if !defined(SSC_INLINE_INCLUDE)
#include "types.hh"
#endif

#if !defined(SSC_INLINE_INCLUDE)
namespace ssc::env {
  using namespace ssc::types;
#endif

  inline auto get (const char* variableName) {
    #if defined(_WIN32)
      char* variableValue = nullptr;
      std::size_t valueSize = 0;
      auto query = _dupenv_s(&variableValue, &valueSize, variableName);

      String result;
      if(query == 0 && variableValue != nullptr && valueSize > 0) {
        result.assign(variableValue, valueSize - 1);
        free(variableValue);
      }

      return result;
    #else
      auto v = getenv(variableName);

      if (v != nullptr) {
        return String(v);
      }

      return String("");
    #endif
  }

  inline auto get (const String& name) {
    return get(name.c_str());
  }

  inline auto set (const char* pair) {
    #if defined(_WIN32)
      return _putenv(pair);
    #else
      return putenv((char*) &pair[0]);
    #endif
  }

  inline auto set (const char* key, const char* value) {
    #if defined(_WIN32)
      SetEnvironmentVariable(key, value);
    #else
      setenv(key, value, 1);
    #endif
  }

  inline auto set (const String& key, const String& value) {
    return set(key.c_str(), value.c_str());
  }

#if !defined(SSC_INLINE_INCLUDE)
}
#endif
#endif
