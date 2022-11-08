module;
#include "../../core/platform-headers.hh"

/**
 * @module ssc.ipc.message
 * @description TODO
 * @example TODO
 */
export module ssc.ipc.message;
import ssc.string;
import ssc.codec;
import ssc.types;

using ssc::codec::decodeURIComponent;
using ssc::string::String;
using ssc::string::split;
using ssc::types::Map;

export namespace ssc::ipc::message {
  #define SSC_INLINE_INCLUDE
  #include "../../core/ipc/message.hh"
}
