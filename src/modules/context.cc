module; // global
#include "../common.hh"

/**
 * @module ssc.context
 * @description Generic context for async JSON based operations.
 * @example
 * import ssc.context;
 * import ssc.runtime;
 * namespace ssc {
 *   using Context = context::Context;
 *   using Runtime = runtime::Runtime;
 *   class MyRuntimeContext : Context {
 *     public:
 *       MyRuntimeContext (Runtime* runtime) : Context(rutime) {
 *         // init
 *       }
 *   };
 * }
 */
export module ssc.context;
import ssc.json;

export namespace ssc::context {
  class Runtime;
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
