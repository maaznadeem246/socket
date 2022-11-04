module; // global
#include "../platform.hh"
#include <map>
#include <memory>
#include <new> // @TODO(jwerle): move this to `platform.hh` or similar
#include <string>

/**
 * @module runtime
 * @description TODO
 * @example
 * import ssc.runtime;
 * import ssc.dns;
 * using Runtime = ssc::runtime::Runtime;
 * using DNS = ssc::dns::DNS;
 * namespace ssc {
 *   Runtime runtime;
 *   runtime.registerInterface<DNS>("dns")
 * }
 */
export module ssc.runtime;
import ssc.javascript;
import ssc.loop;
import ssc.types;
import ssc.string;
import ssc.uv;

using namespace ssc::types;
using ssc::loop::Loop;
using ssc::string::String;

export namespace ssc::runtime {
  class Runtime {
    public:
      class Interface {
        public:
          Runtime *runtime = nullptr;
          Interface () = default;
          ~Interface () {
            this->dispose();
          }

          Interface (Runtime* runtime) {
            this->runtime = runtime;
          }

          void dispose () {}

          template <class T> T* as () {
            return reinterpret_cast<T*>(this);
          }
      };

      struct Interfaces {
        SharedPointer<Interface> dns = nullptr;
      };

      Loop loop;
      Interfaces interfaces;

      Runtime () = default;
      ~Runtime () = default;

      void start () {
        this->loop.init();
        this->loop.start();
      }

      void stop () {
        this->loop.stop();
      }

      void dispatch (Loop::DispatchCallback cb) {
        this->loop.dispatch(cb);
      }

      void wait () {
        this->loop.wait();
      }
  };
}
