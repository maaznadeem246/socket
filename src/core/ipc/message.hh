#ifndef SSC_CORE_IPC_MESSAGE_HH
#define SSC_CORE_IPC_MESSAGE_HH

#include <socket/platform.hh>
#include "../codec.hh"
#include "../string.hh"
#include "../types.hh"

namespace ssc::core::ipc::message {
  using namespace types;
  using codec::decodeURIComponent;
  using string::split;

  struct MessageBuffer {
    char *bytes = nullptr;
    size_t size = 0;
    MessageBuffer () = default;
    MessageBuffer (auto bytes, auto size) {
      this->bytes = bytes;
      this->size = size;
    }
  };

  class Message {
    public:
      using Seq = String;
      String uri = "";
      MessageBuffer buffer;
      String value = "";
      String name = "";
      String seq = "";
      int index = -1;
      Map args;

      Message () = default;
      Message (const Message& message) {
        this->buffer.bytes = message.buffer.bytes;
        this->buffer.size = message.buffer.size;
        this->value = message.value;
        this->index = message.index;
        this->name = message.name;
        this->seq = message.seq;
        this->uri = message.uri;
        this->args = message.args;
      }

      Message (const String& source) {
        String str = source;
        this->uri = source;

        // bail if missing protocol prefix
        if (str.find("ipc://") == -1) return;

        // bail if malformed
        if (str.compare("ipc://") == 0) return;
        if (str.compare("ipc://?") == 0) return;

        String query;
        String path;

        auto raw = split(str, '?');
        path = raw[0];
        if (raw.size() > 1) query = raw[1];

        auto parts = split(path, '/');
        if (parts.size() >= 1) name = parts[1];

        if (raw.size() != 2) return;

        auto pairs = split(raw[1], '&');
        for (auto& rawPair : pairs) {
          auto pair = split(rawPair, '=');
          if (pair.size() <= 1) continue;

          if (pair[0].compare("index") == 0) {
            try {
              this->index = std::stoi(pair[1].size() > 0 ? pair[1] : "0");
            } catch (...) {
              fprintf(stderr, "%s\n", "Warning: received non-integer index");
              continue;
            }
          }

          if (pair[0].compare("value") == 0) {
            this->value = decodeURIComponent(pair[1]);
          }

          if (pair[0].compare("seq") == 0) {
            this->seq = decodeURIComponent(pair[1]);
          }

          args[pair[0]] = pair[1];
        }
      }

      Message (const String& source, char *bytes, size_t size)
        : Message(source)
      {
        this->buffer.bytes = bytes;
        this->buffer.size = size;
      }

      bool has (const String& key) const {
        return this->args.find(key) != this->args.end();
      }

      String get (const String& key) const {
        return this->get(key, "");
      }

      String get (const String& key, const String &fallback) const {
        return args.count(key) ? decodeURIComponent(args.at(key)) : fallback;
      }

      String str () const {
        return this->uri;
      }

      const char * c_str () const {
        return this->uri.c_str();
      }
  };
}
#endif
