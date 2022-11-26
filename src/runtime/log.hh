#ifndef SSC_RUNTIME_LOG_HH
#define SSC_RUNTIME_LOG_HH

#include <socket/socket.hh>
#include <iostream>
#include <string>
#include <stdarg.h>
#include <stdio.h>

#include "runtime.hh"
#include "ipc.hh"

namespace ssc::runtime::log {
  inline auto write (const String& str, bool isError) {
    #if defined(_WIN32)
      StringStream ss;
      ss << str << std::endl;
      auto lineStr = ss.str();

      auto handle = isError ? STD_ERROR_HANDLE : STD_OUTPUT_HANDLE;
      WriteConsoleA(GetStdHandle(handle), lineStr.c_str(), lineStr.size(), NULL, NULL);
    #elif defined(__ANDROID__)
      if (isError) {
        __android_log_print(ANDROID_LOG_ERROR, "", "%s", str.c_str());
      } else {
        __android_log_print(ANDROID_LOG_INFO, "", "%s", str.c_str());
      }
    #else
      (isError ? std::cerr : std::cout) << str;
    #endif
  }

  inline auto info (const String& string) {
    write(string + "\n", false);
  }

  inline auto info (const char* fmt, ...) {
    static char buffer[4096] = {0};
    va_list args;
    va_start(args, fmt);
    memset(buffer, 0, sizeof(buffer));
    vsnprintf(buffer, 4096, fmt, args);
    info(String(buffer));
  }

  inline auto info (std::nullptr_t _) {
    info(String("null"));
  }

  inline auto info (const uint64_t u64) {
    info(std::to_string(u64));
  }

  inline auto info (const int64_t i64) {
    info(std::to_string(i64));
  }

  inline auto info (const int32_t i32) {
    info(std::to_string(i32));
  }

  inline auto info (const double f64) {
    info(std::to_string(f64));
  }

  inline auto info (const float f32) {
    info(JSON::Number(f32).str());
  }

  inline auto info (const JSON::Any& json) {
    if (json.isString()) {
      info(json.as<JSON::String>().value());
    } else {
      info(json.str());
    }
  }

  inline auto info (const Config& config) {
    info(config.str());
  }

  inline auto info (const Script& script) {
    info(script.str());
  }

  inline auto info (const Headers& headers) {
    info(headers.str());
  }

  inline auto info (const Headers::Entries& entries) {
    info(Headers(entries).str());
  }

  inline auto info (const ipc::Result& result) {
    info(result.str());
  }

  inline auto info (const ipc::Message& message) {
    info(message.str());
  }

  inline auto info (const StringStream& stream) {
    info(stream.str());
  }

  inline auto info (bool boolean) {
    info(boolean ? "true" : "false");
  }

  inline auto error (const String& string) {
    write(string + "\n", true);
  }

  inline auto error (const char* fmt, ...) {
    static char buffer[4096] = {0};
    va_list args;
    va_start(args, fmt);
    memset(buffer, 0, sizeof(buffer));
    vsnprintf(buffer, 4096, fmt, args);
    error(String(buffer));
  }

  inline auto error (std::nullptr_t) {
    error(String("null"));
  }

  inline auto error (const int64_t i64) {
    error(std::to_string(i64));
  }

  inline auto error (const int32_t i32) {
    error(std::to_string(i32));
  }

  inline auto error (const double f64) {
    error(JSON::Number(f64).str());
  }

  inline auto error (const float f32) {
    error(JSON::Number(f32).str());
  }

  inline auto error (const JSON::Any& json) {
    if (json.isString()) {
      error(json.as<JSON::String>().value());
    } else {
      error(json.str());
    }
  }

  inline auto error (const Config& config) {
    error(config.str());
  }

  inline auto error (const Script& script) {
    error(script.str());
  }

  inline auto error (const Headers& headers) {
    error(headers.str());
  }

  inline auto error (const Headers::Entries& entries) {
    error(Headers(entries).str());
  }

  inline auto error (const ipc::Result& result) {
    error(result.str());
  }

  inline auto error (const ipc::Message& message) {
    error(message.str());
  }

  inline auto error (const StringStream& stream) {
    error(stream.str());
  }

  inline auto error (bool boolean) {
    error(boolean ? "true" : "false");
  }
}

#endif
