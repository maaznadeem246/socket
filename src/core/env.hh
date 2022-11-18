#ifndef SSC_CORE_ENV_HH
#define SSC_CORE_ENV_HH

#include <socket/socket.hh>
#include "string.hh"

namespace ssc::core::env {
  using namespace ssc::core::string;

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
    #elif defined(__APPLE__) && TARGET_IPHONE_SIMULATOR
      return String("");
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
    #elif defined(__APPLE__) && TARGET_IPHONE_SIMULATOR
      return 0;
    #else
      return putenv((char*) &pair[0]);
    #endif
  }

  inline auto set (const char* key, const char* value) {
    #if defined(_WIN32)
      SetEnvironmentVariable(key, value);
    #elif defined(__APPLE__) && TARGET_IPHONE_SIMULATOR
      return 0;
    #else
      setenv(key, value, 1);
    #endif
  }

  inline auto set (const String& key, const String& value) {
    return set(key.c_str(), value.c_str());
  }

  inline auto set (const char* key, const int value) {
    return set(key, std::to_string(value));
  }

  inline auto set (const char* key, const bool value) {
    return set(key, value ? "true" : "false");
  }

  inline auto has (const String& key) {
    return get(key).size() != 0;
  }

  inline auto has (const char*  key) {
    return get(key).size() != 0;
  }
}
#endif
