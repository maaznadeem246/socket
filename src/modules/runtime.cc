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
import ssc.fs;
import ssc.os;
import ssc.peer;
import ssc.platform;
import ssc.string;
import ssc.timers;
import ssc.types;
import ssc.udp;

using ssc::data::DataManager;
using ssc::dns::DNS;
using ssc::loop::Loop;
using ssc::fs::DescriptorManager;
using ssc::fs::FS;
using ssc::os::OS;
using ssc::peer::PeerManager;
using ssc::platform::Platform;
using ssc::string::String;
using ssc::timers::Timers;
using ssc::types::SharedPointer;
using ssc::udp::UDP;

export namespace ssc::runtime {
  class Runtime {
    public:
      DescriptorManager descriptorManager;
      DataManager dataManager;
      PeerManager peerManager;
      Timers timers;
      Loop loop;

      DNS dns;
      FS fs;
      OS os;
      Platform platform;
      UDP udp;

      Runtime (const Runtime&) = delete;

      Runtime ()
        : descriptorManager(this->loop, &this->fs),
          peerManager(this->loop),
          timers(this->loop),
          dns(this->loop),
          fs(this->loop, this->descriptorManager),
          os(this->loop, this->peerManager),
          platform(this->loop),
          udp(this->loop, this->peerManager)
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
