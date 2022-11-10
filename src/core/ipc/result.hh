#ifndef SSC_CORE_IPC_RESULT_HH
#if !defined(SSC_INLINE_INCLUDE)
#define SSC_CORE_IPC_RESULT_HH
#include "../codec.hh"
#include "../json.hh"
#include "../types.hh"
#include "message.hh"
#include "data.hh"
#endif

#if !defined(SSC_INLINE_INCLUDE)
namespace ssc::ipc::result {
#endif
  #if !defined(SSC_INLINE_INCLUDE)
  using namespace ssc::types;
  #endif

  struct Value {
    data::Data data;
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
      ) : Result(seq, message, json, data::Data{}) {
        // noop
      }

      Result (
        const message::Message::Seq& seq,
        const message::Message& message,
        JSON::Any json,
        data::Data data
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

#if !defined(SSC_INLINE_INCLUDE)
}
#endif

#endif
