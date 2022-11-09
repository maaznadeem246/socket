module;
#include "../core/platform.hh"

/**
 * @module ssc.string
 * @description Various utiltity string functions
 * @example
 * import ssc.string;
 * import ssc.types;
 * namespace ssc {
 *   using ssc::string::String;
 *   using ssc::string::split;
 *   using ssc::string::trim;
 *   using ssc::types::Vector;
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
  using String = ssc::types::String;
  using StringStream = ssc::types::StringStream;
  using WString = ssc::types::WString;

  #define SSC_INLINE_INCLUDE
  #include "../core/string.hh"
}
