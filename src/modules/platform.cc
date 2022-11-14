module;
#include "../core/platform.hh"

export module ssc.platform;
import ssc.data;
import ssc.loop;
import ssc.json;
import ssc.string;
import ssc.types;

using namespace ssc::data;
using namespace ssc::loop;
using namespace ssc::string;
using namespace ssc::types;

export namespace ssc::platform {
  using core::platform::CorePlatform;
  class Platform : public CorePlatform {
    public:
      using Callback = Function<void(String, JSON::Any, Data)>;
      struct RequestContext {
        String seq;
        Callback callback;
        RequestContext (const String& seq, Callback callback) {
          this->seq = seq;
          this->callback = callback;
        }
      };

      Loop& loop;
      Platform (Loop& loop) : loop(loop) {}
      void event (
        const String seq,
        const String event,
        const String data,
        Callback callback
      ) {
        this->loop.dispatch([=, this]() {
          /* FIXIME
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

          callback(seq, json, Data{});
        */
        });
      }

      void notify (
        const String seq,
        const String title,
        const String body,
        Callback callback
      ) {
        CorePlatform::notify(title, body, [&](auto error) {
          auto json = JSON::Object();
          json["source"] = "platform.notify";

          if (error.size() > 0) {
            json["err"] = JSON::Object::Entries {
              {"message", error}
            };
          }

          callback(seq, json, Data{});
        });
      }

      void openExternal (
        const String seq,
        const String value,
        Callback callback
      ) {
        CorePlatform::openExternal(value, [&](auto error) {
          auto json = JSON::Object();
          json["source"] = "platform.openExternal";

          if (error.size() > 0) {
            json["err"] = JSON::Object::Entries {
              {"message", error}
            };
          }

          callback(seq, json, Data{});
        });
      }
  };
}
