module;
#include "../../core/platform.hh"

/**
 * @module ssc.ipc.data
 * @description TODO
 * @example TODO
 */
export module ssc.ipc.data;
import ssc.javascript;
import ssc.headers;
import ssc.string;
import ssc.types;
import ssc.utils;
import ssc.json;

using ssc::headers::Headers;
using ssc::string::String;
using ssc::string::trim;
using ssc::types::Lock;
using ssc::types::Mutex;
using ssc::types::SharedPointer;
using ssc::types::Vector;
using ssc::utils::rand64;

export namespace ssc::ipc::data {
  #define SSC_INLINE_INCLUDE
  #include "../../core/ipc/data.hh"
}
