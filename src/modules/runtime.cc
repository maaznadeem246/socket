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

      struct {
        Loop main;
        Loop background;
      } loops;

      DNS dns;
      FS fs;
      OS os;
      Platform platform;
      UDP udp;

      Runtime (const Runtime&) = delete;

      Runtime ()
        : timers(this->loops.background),
          descriptorManager(this->loops.main, this->timers, &this->fs),
          peerManager(this->loops.main),
          dns(this->loops.main),
          fs(this->loops.main, this->descriptorManager, this->timers),
          os(this->loops.main, this->peerManager),
          platform(this->loops.main),
          udp(this->loops.main, this->peerManager)
      {}

      void start () {
        this->loops.main.start();
        this->loops.background.start();

        this->loops.main.dispatch([&] () mutable {
          this->descriptorManager.init();
          this->timers.start();
        });
      }

      void stop () {
        this->loops.main.stop();
        this->loops.background.stop();
        this->timers.stop();
      }

      void dispatch (Loop::DispatchCallback cb) {
        this->loops.main.dispatch(cb);
      }

      void wait () {
        this->loops.main.wait();
      }
  };
}
