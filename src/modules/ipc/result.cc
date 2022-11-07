export module ssc.ipc.result;
import ssc.ipc.message;
import ssc.string;
import ssc.types;
import ssc.json;

using ssc::ipc::message::Message;
using ssc::string::String;
using ssc::types::Post;

export namespace ssc::ipc::result {
  class Result {
    public:
      class Err {
        public:
          Message message;
          Message::Seq seq;
          JSON::Any value;
          Err () = default;
          Err (const Message& message, JSON::Any value) {
            this->seq = message.seq;
            this->message = message;
            this->value = value;
          }
      };

      class Data {
        public:
          Message message;
          Message::Seq seq;
          JSON::Any value;
          Post post;

          Data () = default;
          Data (const Message& message, JSON::Any value, Post post) {
            this->seq = message.seq;
            this->message = message;
            this->value = value;
            this->post = post;
          }

          Data (
            const Message& message,
            JSON::Any value
          ) : Data(message, value, Post{}) {
            // noop
          }

      };

      Message message;
      Message::Seq seq;
      String source = "";
      JSON::Any value = nullptr;
      JSON::Any data = nullptr;
      JSON::Any err = nullptr;
      Post post;

      Result () = default;
      Result (const Err error) {
        this->err = error.value;
      }

      Result (const Data data) {
        this->data = data.value;
      }

      Result (const Message::Seq& seq, const Message& message) {
        this->message = message;
        this->source = message.name;
        this->seq = seq;
      }

      Result (
        const Message::Seq& seq,
        const Message& message,
        JSON::Any json
      ) : Result(seq, message, json, Post{}) {
        // noop
      }

      Result (
        const Message::Seq& seq,
        const Message& message,
        JSON::Any json,
        Post post
      ) : Result(seq, message) {
        this->post = post;
        if (json.type != JSON::Type::Any) {
          this->value = json;
        }
      }

      auto str () const {
        return this->json().str();
      }

      JSON::Any json () const {
        // return result value if set explicitly
        if (this->value.type != JSON::Type::Null) {
          return this->value;
        }

        auto entries = JSON::Object::Entries {
          {"source", this->source},
          {"data", this->data},
          {"err", this->err}
        };

        return JSON::Object(entries);
      }
  };
}
