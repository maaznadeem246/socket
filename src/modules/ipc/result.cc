module;
#include "../../core/platform.hh"

/**
 * @module ssc.ipc.result;
 * @description TODO
 * @example TODO
 */
export module ssc.ipc.result;
import ssc.ipc.message;
import ssc.ipc.data;
import ssc.types;
import ssc.json;

using namespace ssc::types;

export namespace ssc::ipc::result {
  #define SSC_INLINE_INCLUDE
  #include "../../core/ipc/result.hh"
}
