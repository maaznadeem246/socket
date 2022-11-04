module; // global
#include <functional>

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
import ssc.runtime;
import ssc.string;
import ssc.types;
import ssc.json;

using ssc::runtime::Runtime;
using ssc::string::String;
using ssc::types::Post;

export namespace ssc::context {
  class Context : public Runtime::Interface {
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

      Context (Runtime* runtime) : Interface(runtime) {}
  };
}
