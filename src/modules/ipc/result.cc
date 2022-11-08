module;
#include "../../core/platform.hh"
export module ssc.ipc.result;
import ssc.ipc.message;
import ssc.string;
import ssc.types;
import ssc.json;

using ssc::ipc::message::Message;
using ssc::string::String;
using ssc::types::Post;

export namespace ssc::ipc::result {
  #define SSC_INLINE_INCLUDE
  #include "../../core/ipc/result.hh"
}
