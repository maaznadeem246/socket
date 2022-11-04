module; // global

/**
 * @module ssc.init
 * @description TODO
 * @example
 */
export module ssc.init;
import ssc.runtime;
import ssc.types;
import ssc.dns;

using ssc::types::SharedPointer;
using ssc::runtime::Runtime;
using ssc::dns::DNS;

export namespace ssc {
  void init (Runtime* runtime, const int argc, const char** argv) {
    auto dns = new DNS(runtime);
    runtime->interfaces.dns = SharedPointer<Runtime::Interface>(reinterpret_cast<Runtime::Interface*>(dns));
  }

  void init (Runtime* runtime) {
    init(runtime, 0, (const char**) "");
  }
}
