#include "core.hh"
#include "json.hh"

namespace SSC {
  void Core::Diagnostics::start (String& seq, Module::Callback cb) {
    this->channel.start(seq, cb);
  }

  void Core::Diagnostics::stop (String& seq, Module::Callback cb) {
    this->channel.stop(seq, cb);
  }

  void Core::Diagnostics::publish (
    String& seq,
    UDP::SendOptions options,
    Module::Callback cb
  ) {
    this->channel.publish(seq, options.bytes, options.size, cb);
  }

  Core::Diagnostics::Channel::Channel (Core* core) {
    auto PORT_FROM_ENV = getEnv("SOCKET_SHARED_DIAGNOSTICS_CHANNEL_PORT");
    int port = 0;
    if (PORT_FROM_ENV.size() > 0) {
      try {
        port = std::stoi(PORT_FROM_ENV);
      } catch (...) {}
    }
    this->id = 0;
    this->core = core;
    this->options = UDP::BindOptions {
    #if defined(__WIN32)
      "127.0.0.1",
    #else
      "0.0.0.0",
    #endif
      port,
      true
    };
  }

  void Core::Diagnostics::Channel::start (String& seq, Module::Callback cb) {
    this->id = rand64();
    this->core->udp.bind(seq, this->id, this->options, [=](auto seq, JSON::Any json, auto post) {
      auto object = json.as<JSON::Object>();
      object["context"] = "udp.bind";

      if (object.has("err")) {
        cb(seq, json, post);
        return;
      }

      this->core->udp.readStart(seq, this->id, [cb, bindjson = object](auto seq, JSON::Any json, auto post) {
        auto object = json.as<JSON::Object>();
        object["context"] = "udp.readStart";

        if (object.has("err")) {
          cb(seq, object, post);
        } else if (seq.size() > 0 && seq != "-1") {
          cb(seq, bindjson, post);
        } else {
          cb(seq, object, post);
        }
      });
    });
  }

  void Core::Diagnostics::Channel::stop (String& seq, Module::Callback cb) {
    this->core->udp.close(seq, this->id, [=](auto seq, auto json, auto post) {
      auto object = json.template as<JSON::Object>();
      this->id = 0;
      cb(seq, json, post);
    });
  }

  void Core::Diagnostics::Channel::publish (
    String& seq,
    char* bytes,
    size_t size,
    Module::Callback cb
  ) {
    auto options = UDP::SendOptions {
      this->options.address,
      this->options.port,
      bytes,
      size
    };

    this->core->udp.send(seq, this->id, options, cb);
  }
}
