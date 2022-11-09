module;
#include "../core/platform.hh"

/**
 * @module ssc.ip
 * @description TODO
 * @example
 * TODO
 */
export module ssc.ip;
import ssc.string;
using String = ssc::string::String;
export namespace ssc::ip {
  #define SSC_INLINE_INCLUDE
  #include "../core/ip.hh"
}
