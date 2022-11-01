module; // global
#include "../platform.hh"

/**
 * @module runtime
 * @description TODO
 * @example
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
          Interface (Runtime* runtime) {
            this->runtime = runtime;
          }
      };

      using InterfaceMap = std::map<String, Interface>;
      InterfaceMap interfaces;
      Loop loop;

      Runtime () = default;

      void addInterface (const String& name, Interface runtimeInterface) {
        this->interfaces[name] = runtimeInterface;
      }

      template <typename T> T* getInterface (const String& name) {
        if (this->interfaces.find(name) != this->interfaces.end()) {
          return reinterpret_cast<T*>(&this->interfaces[name]);
        }

        return nullptr;
      }
  };
}
