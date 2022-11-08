module;

#include "../core/internal.hh"
#include "../core/platform.hh"
#include <string>

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
using ssc::types::Post;
using ssc::loop::Loop;

export namespace ssc::platform {
  struct PlatformInfo {
    #if defined(__x86_64__) || defined(_M_X64)
      const String arch = "x86_64";
    #elif defined(__aarch64__) || defined(_M_ARM64)
      const String arch = "arm64";
    #else
      const String arch = "unknown";
    #endif

    #if defined(_WIN32)
      const String os = "win32";
      bool mac = false;
      bool ios = false;
      bool win = true;
      bool linux = false;
      bool unix = false;
    #elif defined(__APPLE__)
      bool win = false;
      bool linux = false;
      #if TARGET_OS_IPHONE || TARGET_IPHONE_SIMULATOR
        String os = "ios";
        bool ios = true;
        bool mac = false;
      #else
        String os = "mac";
        bool ios = false;
        bool mac = true;
      #endif
      #if defined(__unix__) || defined(unix) || defined(__unix)
        bool unix = true;
      #else
        bool unix = false;
      #endif
    #elif defined(__linux__)
      #undef linux
      #ifdef __ANDROID__
        const String os = "android";
      #else
        const String os = "linux";
      #endif
      bool mac = false;
      bool ios = false;
      bool win = false;
      bool linux = true;
      #if defined(__unix__) || defined(unix) || defined(__unix)
        bool unix = true;
      #else
        bool unix = false;
      #endif
    #elif defined(__FreeBSD__)
      const String os = "freebsd";
      bool mac = false;
      bool ios = false;
      bool win = false;
      bool linux = false;
      #if defined(__unix__) || defined(unix) || defined(__unix)
        bool unix = true;
      #else
        bool unix = false;
      #endif
    #elif defined(BSD)
      const String os = "openbsd";
      bool ios = false;
      bool mac = false;
      bool win = false;
      #if defined(__unix__) || defined(unix) || defined(__unix)
        bool unix = true;
      #else
        bool unix = false;
      #endif
    #endif
  };

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
