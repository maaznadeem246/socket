#ifndef SSC_CORE_IPC_RESULT_HH
#define SSC_CORE_IPC_RESULT_HH

#include <socket/platform.hh>
#include "../codec.hh"
#include "../data.hh"
#include "../json.hh"
#include "../types.hh"
#include "message.hh"

namespace ssc::core::ipc::result {
  using namespace types;
  using ssc::core::data::CoreData;

  struct Value {
    data::CoreData data;
    struct {
      JSON::Any data;
      JSON::Any err;
      JSON::Any raw;
    } json;
  };

  class Result {
    public:
      class Err {
        public:
          message::Message message;
          message::Message::Seq seq;
          JSON::Any json;
          Err () = default;
          Err (const message::Message& message, const JSON::Any& json) {
            this->seq = message.seq;
            this->message = message;
            this->json = json;
          }
      };

      class Data {
        public:
          message::Message message;
          message::Message::Seq seq;
          JSON::Any json;

          Data () = default;
          Data (const message::Message& message, const JSON::Any& json) {
            this->seq = message.seq;
            this->message = message;
            this->json = json;
          }
      };

      message::Message message;
      message::Message::Seq seq;
      String source = "";
      Value value;

      Result () = default;
      Result (const Err& error) {
        this->value.json.err = error.json;
      }

      Result (const Data& data) {
        this->value.json.data = data.json;
      }

      Result (const message::Message::Seq& seq, const message::Message& message) {
        this->message = message;
        this->source = message.name;
        this->seq = seq;
      }

      Result (
        const message::Message::Seq& seq,
        const message::Message& message,
        JSON::Any json
      ) : Result(seq, message, json, data::CoreData{}) {
        // noop
      }

      Result (
        const message::Message::Seq& seq,
        const message::Message& message,
        JSON::Any json,
        CoreData data
      ) : Result(seq, message) {
        this->value.data = data;
        if (json.type != JSON::Type::Any) {
          this->value.json.raw = json;
        }
      }

      auto str () const {
        return this->json().str();
      }

      JSON::Any json () const {
        // return result value if set explicitly
        if (this->value.json.raw.type != JSON::Type::Null) {
          return this->value.json.raw;
        }

        auto entries = JSON::Object::Entries {
          {"source", this->source},
          {"data", this->value.json.data},
          {"err", this->value.json.err}
        };

        return JSON::Object(entries);
      }
  };
}
#endif
