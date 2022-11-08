module;

#include "../core/platform.hh"

/**
 * @module ssc.string
 * @description Various utiltity string functions
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
