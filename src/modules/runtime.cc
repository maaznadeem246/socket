module; // global
#include "../platform.hh"

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
import ssc.uv;

using Loop = ssc::loop::Loop;

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
      };

      using InterfaceMap = std::map<String, SharedPointer<Interface>>;
      InterfaceMap interfaces;
      Loop loop;

      Runtime () = default;
      ~Runtime () = default;

      template <typename T, typename ...Args> void registerInterface (
        const String& name,
        Args ...args
      ) {
        static_assert(std::is_base_of<Interface, T>::value);
        this->interfaces[name] = SharedPointer<T>(new T(this, args...));
      }

      template <typename T> T* getInterface (const String& name) {
        if (this->interfaces.find(name) != this->interfaces.end()) {
          return reinterpret_cast<T*>(this->interfaces[name].get());
        }

        return nullptr;
      }
  };
}
