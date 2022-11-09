module;
#include "../core/platform.hh"

/**
 * @module ssc.codec
 * @description Various utiltity encode/decode functions
 * @example
 * TODO
 */
export module ssc.codec;
import ssc.string;
import ssc.types;

using ssc::string::String;

export namespace ssc::codec {
  #define SSC_INLINE_INCLUDE
  #include "../core/codec.hh"
}
