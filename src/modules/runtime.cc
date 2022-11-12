module; // global
#include <socket/platform.hh>

#include <functional>
#include <new>

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
import ssc.string;
import ssc.types;
import ssc.loop;
import ssc.ipc;
import ssc.dns;

using ssc::dns::DNS;
using ssc::loop::Loop;
using ssc::string::String;
using ssc::types::SharedPointer;

export namespace ssc::runtime {
  class Runtime {
    public:
      Loop loop;
      DNS dns;

      Runtime (const Runtime&) = delete;
      Runtime () : dns(this->loop) {
      }
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
