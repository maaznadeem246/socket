module;
#include "../core/platform.hh"

/**
 * @module ssc.headers
 * @description New line delimited key-value headers utilities.
 * @example
 * TODO
 */
export module ssc.headers;
import ssc.string;
import ssc.types;
import ssc.json;

using ssc::string::split;
using ssc::string::String;
using ssc::string::StringStream;
using ssc::types::Map;
using ssc::types::Vector;

export namespace ssc::headers {
  #define SSC_INLINE_INCLUDE
  #include "../core/headers.hh"
}
