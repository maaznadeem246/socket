module;

#include "../platform.hh"

export module ssc.runtime:platform;

export namespace ssc {
  class Platform : public Module {
    public:
      Platform (auto core) : Module(core) {}
      void event (
        const String seq,
        const String event,
        const String data,
        Module::Callback cb
      );
      void notify (
        const String seq,
        const String title,
        const String body,
        Module::Callback cb
      );
      void openExternal (
        const String seq,
        const String value,
        Module::Callback cb
      );
  };
}
