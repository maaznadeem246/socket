module;

#if defined(__APPLE__)
  #include <TargetConditionals.h>
#endif

export module ssc.platform;
import ssc.context;
import ssc.string;
import ssc.types;
import ssc.loop;
import ssc.json;

using ssc::context::Context;
using ssc::string::String;
using ssc::types::Mutex;
using ssc::types::Lock;
using ssc::loop::Loop;

export namespace ssc::platform {

  class Platform : public Context {
    public:
      Platform (Loop& loop) : Context(loop) {}
      void event (
        const String seq,
        const String event,
        const String data,
        Context::Callback cb
      );

      void notify (
        const String seq,
        const String title,
        const String body,
        Context::Callback cb
      ) {
        /*
        internal::platform::notify(
          this->internal,
          title,
          body,
          [&](auto error) {
            auto json = JSON::Object();
            json["source"] = "platform.notify";

            if (error.size() > 0) {
              json["err"] = JSON::Object::Entries {
                {"message", error}
              };
            }

            cb(seq, json, Post{});
          });
          */
      }

      void openExternal (
        const String seq,
        const String value,
        Context::Callback cb
      ) {
        /*
        internal::platform::openExternal(
          this->internal,
          value,
          [&](auto error) {
            auto json = JSON::Object();
            json["source"] = "platform.openExternal";

            if (error.size() > 0) {
              json["err"] = JSON::Object::Entries {
                {"message", error}
              };
            }

            cb(seq, json, Post{});
          });
          */
      }
  };

  void Platform::event (
    const String seq,
    const String event,
    const String data,
    Context::Callback cb
  ) {
    /*
    this->runtime->dispatch([=, this]() {
      auto fs = this->runtime->interfaces.fs.get().as<FS>();
      // init page
      if (event == "domcontentloaded") {
        Lock lock(this->runtime->fs.mutex);

        for (auto const &tuple : this->runtime->fs.descriptors) {
          auto desc = tuple.second;
          if (desc != nullptr) {
            Lock lock(desc->mutex);
            desc->stale = true;
          } else {
            this->runtime->fs.descriptors.erase(tuple.first);
          }
        }
      }

      auto json = JSON::Object::Entries {
        {"source", "event"},
        {"data", JSON::Object::Entries{}}
      };

      cb(seq, json, Post{});
    });
    */
  }
}
