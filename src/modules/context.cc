/**
 * Global module fragment.
 */
module;

#include "../common.hh"

/**
 * `ssc.runtime:context` module fragment.
 */
export module ssc.runtime:context;
import :runtime;
import :json;

export namespace ssc {
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
