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
    this->channel.publish(seq, options, cb);
  }

  Core::Diagnostics::Channel::Channel (Core* core) {
    this->id = 0;
    this->core = core;
    this->options = UDP::BindOptions {
    #if defined(__WIN32)
      "127.0.0.1",
    #else
      "0.0.0.0",
    #endif
      32879,
      true
    };
  }

  void Core::Diagnostics::Channel::start (String& seq, Module::Callback cb) {
    this->id = rand64();
    this->core->udp.bind(seq, this->id, this->options, [=](auto seq, auto json, auto post) {
      if (!json.template as<JSON::Object>().has("err")) {
        this->core->udp.readStart(seq, this->id, [cb](auto seq, auto json, auto post) {
          auto object = json.template as<JSON::Object>();
          object["context"] = "diagnostics";
          cb(seq, json, post);
        });
      }

      debug("json= %s", json.str().c_str());
      cb(seq, json, post);
    });
  }

  void Core::Diagnostics::Channel::stop (String& seq, Module::Callback cb) {
    this->core->udp.close(seq, this->id, [=](auto seq, auto json, auto post) {
      this->id = 0;
      cb(seq, json, post);
    });
  }

  void Core::Diagnostics::Channel::publish (
    String& seq,
    UDP::SendOptions options,
    Module::Callback cb
  ) {
    this->core->udp.send(seq, this->id, options, cb);
  }
}
