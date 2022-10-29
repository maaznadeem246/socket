/**
 * Global module fragment.
 */
module;

#include "../common.hh"

/**
 * `ssc.runtime:context` module fragment.
 */
export module context;
import json;

export namespace ssc::context {
  class Runtime;
  class Context {
    public:
      using Callback = std::function<void(String, JSON::Any, Post)>;
      struct RequestContext {
        String seq;
        Callback cb;
        RequestContext () = default;
        RequestContext (String seq, Callback cb) {
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
