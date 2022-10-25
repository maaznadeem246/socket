module;

#include "../common.hh"

import :interfaces;
import :json;

export module ssc.runtime:module;

export namespace ssc {
  class Module {
    public:
      using Callback = std::function<void(String, JSON::Any, Post)>;
      struct RequestContext {
        String seq;
        Module::Callback cb;
        RequestContext () = default;
        RequestContext (String seq, Module::Callback cb) {
          this->seq = seq;
          this->cb = cb;
        }
      };

      Runtime *runtime = nullptr;
      Module (Runtime* runtime) {
        this->runtime = runtime;
      }
  };
}
