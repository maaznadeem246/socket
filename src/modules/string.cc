module;

#include "../core/types-headers.hh"

/**
 * @module ssc.string
 * @description Various utiltity string functions
 */
export module ssc.string;
import ssc.types;

export namespace ssc::string {
  using String = ssc::types::String;
  using StringStream = ssc::types::StringStream;
  using WString = ssc::types::WString;

  #define SSC_INLINE_INCLUDE
  #include "../core/string.hh"
}
