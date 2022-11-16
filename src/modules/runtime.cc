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
import ssc.data;
import ssc.dns;
import ssc.loop;
import ssc.os;
import ssc.peer;
import ssc.platform;
import ssc.string;
import ssc.types;
import ssc.udp;

using ssc::data::DataManager;
using ssc::dns::DNS;
using ssc::loop::Loop;
using ssc::os::OS;
using ssc::peer::PeerManager;
using ssc::platform::Platform;
using ssc::string::String;
using ssc::types::SharedPointer;
using ssc::udp::UDP;

export namespace ssc::runtime {
  class Runtime {
    public:
      DataManager dataManager;
      PeerManager peerManager;
      Loop loop;

      DNS dns;
      OS os;
      Platform platform;
      UDP udp;

      Runtime (const Runtime&) = delete;

      Runtime ()
        : peerManager(this->loop),
          dns(this->loop),
          os(this->loop, peerManager),
          platform(this->loop),
          udp(this->loop, peerManager)
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
