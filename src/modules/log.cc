module; // global
#include <iostream>
#include <stdio.h>
#include <string>

/**
 * @module ssc.log
 * @description Logging functions for stdout/stderr
 * @example
 * import ssc.log;
 * namespace ssc {
 *   log::info("logged to stdout");
 *   log::error("logged to stderr");
 * }
 */
export module ssc.log;
import ssc.string;
import ssc.json;

using String = ssc::string::String;

export namespace ssc::log {
  inline auto write (const String& str, bool isError) {
    #if defined(_WIN32)
      StringStream ss;
      ss << str << std::endl;
      auto lineStr = ss.str();

      auto handle = isError ? STD_ERROR_HANDLE : STD_OUTPUT_HANDLE;
      WriteConsoleA(GetStdHandle(handle), lineStr.c_str(), lineStr.size(), NULL, NULL);
    #else
      (isError ? std::cerr : std::cout) << str << std::endl;
    #endif
  }

  auto info (const String& string) {
    write(string, false);
  }

  auto info (const JSON::Any& json) {
    info(json.str());
  }

  auto error (const String& string) {
    write(string, true);
  }

  auto error (const JSON::Any& json) {
    error(json.str());
  }
}
