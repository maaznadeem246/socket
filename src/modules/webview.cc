module;
#include "../core/internal/webview.hh"

/**
 */
export module ssc.webview;
import ssc.headers;
import ssc.string;
import ssc.types;
import ssc.json;
import ssc.ipc;

using namespace ssc::types;
using ssc::headers::Headers;
using ssc::ipc::data::DataManager;
using ssc::ipc::message::Message;
using ssc::string::String;

export namespace ssc::webview {
  #define SSC_INLINE_INCLUDE
  #include "../core/webview.hh"
}
