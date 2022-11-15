module; // global
#include <memory>

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
import ssc.dns;
import ssc.loop;
import ssc.platform;
import ssc.string;
import ssc.types;

using ssc::dns::DNS;
using ssc::loop::Loop;
using ssc::platform::Platform;
using ssc::string::String;
using ssc::types::SharedPointer;

export namespace ssc::runtime {
  class Runtime {
    public:
      Loop loop;
      Platform platform;
      DNS dns;

      Runtime ()
        : dns(this->loop),
          platform(this->loop)
      { }

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
