export module ssc.runtime:context;
import :interfaces;
import :json;

#include "../common.hh"

export namespace ssc {
  class Context {
    public:
      using Callback = std::function<void(String, JSON::Any, Post)>;
      struct RequestContext {
        String seq;
        Context::Callback cb;
        RequestContext () = default;
        RequestContext (String seq, Context::Callback cb) {
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
