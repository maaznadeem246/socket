module;
#include "../common.hh"

/**
 * @module context
 * @description Generic context for async JSON based operations.
 * @example
 * import context;
 * using namespace ssc::context;
 * TODO
 */
export module context;
import runtime;
import json;

using Runtime = ssc::runtime::Runtime;

export namespace ssc::context {
  class Context {
    public:
      using Callback = std::function<void(String, JSON::Any, Post)>;
      struct RequestContext {
        String seq;
        Callback cb;
        RequestContext () = default;
        RequestContext (const String& seq, Callback cb) {
          this->seq = seq;
          this->cb = cb;
        }
      };

      Runtime *runtime = nullptr;
      Context (Runtime* runtime) {
        this->runtime = runtime;
      }
  };
}
