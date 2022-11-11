module;
#include <socket/platform.hh>
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
import ssc.string;
import ssc.types;
import ssc.json;
import ssc.loop;

using ssc::string::String;
using ssc::types::Post;
using ssc::loop::Loop;

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

      Loop& loop;
      Context (Loop& loop) : loop(loop) {}
  };
}
