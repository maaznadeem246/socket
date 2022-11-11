module;
#include <string>
#include "../core/string.hh"

/**
 * @module ssc.string
 * @description Various utiltity string functions
 * @example
 * import ssc.string;
 * import ssc.types;
 * namespace ssc {
 *   using namespace ssc:string;
 *   using namespace ssc::types;
 *
 *   String trimmed = trim("  hello world  "); // "hello world"
 *   String replaced = replace(trimmed, "hello", "good byte"); // "good byte world"
 *   Vector<String> parts = split(trimmed); // {"h", "e", "l", "l", "o", " ", "w", "o", "r", "l", "d"};
 * }
 */
export module ssc.string;
import ssc.types;

using namespace ssc::types;

export namespace ssc::string {
  using ssc::types::String;
  using ssc::types::StringStream;
  using ssc::types::WString;

  using ssc::core::string::replace; // NOLINT(misc-unused-using-decls)
  using ssc::core::string::replaceAll; // NOLINT(misc-unused-using-decls)
  using ssc::core::string::split; // NOLINT(misc-unused-using-decls)
  using ssc::core::string::splitc; // NOLINT(misc-unused-using-decls)
  using ssc::core::string::StringToWString; // NOLINT(misc-unused-using-decls)
  using ssc::core::string::tmpl; // NOLINT(misc-unused-using-decls)
  using ssc::core::string::toString; // NOLINT(misc-unused-using-decls)
  using ssc::core::string::ToString; // NOLINT(misc-unused-using-decls)
  using ssc::core::string::ToWString; // NOLINT(misc-unused-using-decls)
  using ssc::core::string::trim; // NOLINT(misc-unused-using-decls)
  using ssc::core::string::WStringToString; // NOLINT(misc-unused-using-decls)

  template <typename ...Args> auto format (const String& fmt, Args... args) {
    return ssc::core::string::format(fmt, args...);
  }
}
