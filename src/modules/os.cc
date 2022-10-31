module; // global
#include "../platform.hh"

/**
 * @module javascript
 * @description TODO
 * @example
 * import javascript;
 * using namespace ssc::javascript;
 * TODO
 */
export module os;
import context;
import runtime;
import json;
import uv;

using namespace ssc::context;

export namespace ssc {
  class OS : public Context {
    public:
      static const int RECV_BUFFER = 1;
      static const int SEND_BUFFER = 0;

      OS (auto runtime) : Context(runtime) {}
      void bufferSize (
        const String seq,
        uint64_t peerId,
        size_t size,
        int buffer,
        Context::Callback cb
      );
      void networkInterfaces (const String seq, Context::Callback cb) const;
  };

  void OS::networkInterfaces (
    const String seq,
    Context::Callback cb
  ) const {
    uv_interface_address_t *infos = nullptr;
    StringStream value;
    StringStream v4;
    StringStream v6;
    int count = 0;

    int rc = uv_interface_addresses(&infos, &count);

    if (rc != 0) {
      auto json = JSON::Object(JSON::Object::Entries {
        {"source", "os.networkInterfaces"},
        {"err", JSON::Object::Entries {
          {"type", "InternalError"},
          {"message",
            String("Unable to get network interfaces: ") + String(uv_strerror(rc))
          }
        }}
      });

      return cb(seq, json, Post{});
    }

    JSON::Object::Entries ipv4;
    JSON::Object::Entries ipv6;
    JSON::Object::Entries data;

    for (int i = 0; i < count; ++i) {
      uv_interface_address_t info = infos[i];
      struct sockaddr_in *addr = (struct sockaddr_in*) &info.address.address4;
      char mac[18] = {0};
      snprintf(mac, 18, "%02x:%02x:%02x:%02x:%02x:%02x",
        (unsigned char) info.phys_addr[0],
        (unsigned char) info.phys_addr[1],
        (unsigned char) info.phys_addr[2],
        (unsigned char) info.phys_addr[3],
        (unsigned char) info.phys_addr[4],
        (unsigned char) info.phys_addr[5]
      );

      if (addr->sin_family == AF_INET) {
        JSON::Object::Entries entries;
        entries["internal"] = info.is_internal == 0 ? "false" : "true";
        entries["address"] = addrToIPv4(addr);
        entries["mac"] = String(mac, 17);
        ipv4[String(info.name)] = entries;
      }

      if (addr->sin_family == AF_INET6) {
        JSON::Object::Entries entries;
        entries["internal"] = info.is_internal == 0 ? "false" : "true";
        entries["address"] = addrToIPv6((struct sockaddr_in6*) addr);
        entries["mac"] = String(mac, 17);
        ipv6[String(info.name)] = entries;
      }
    }

    uv_free_interface_addresses(infos, count);

    data["ipv4"] = ipv4;
    data["ipv6"] = ipv6;

    auto json = JSON::Object::Entries {
      {"source", "os.networkInterfaces"},
      {"data", data}
    };

    cb(seq, json, Post{});
  }

  void OS::bufferSize (
    const String seq,
    uint64_t peerId,
    size_t size,
    int buffer,
    Context::Callback cb
  ) {
    if (buffer < 0) {
      buffer = OS::SEND_BUFFER;
    } else if (buffer > 1) {
      buffer = OS::RECV_BUFFER;
    }

    this->runtime->dispatchEventLoop([=, this]() {
      auto peer = this->runtime->getPeer(peerId);

      if (peer == nullptr) {
        auto json = JSON::Object::Entries {
          {"source", "bufferSize"},
          {"err", JSON::Object::Entries {
            {"id", std::to_string(peerId)},
            {"code", "NOT_FOUND_ERR"},
            {"type", "NotFoundError"},
            {"message", "No peer with specified id"}
          }}
        };

        cb(seq, json, Post{});
        return;
      }

      Lock lock(peer->mutex);
      auto handle = (uv_handle_t*) &peer->handle;
      auto err = buffer == RECV_BUFFER
       ? uv_recv_buffer_size(handle, (int *) &size)
       : uv_send_buffer_size(handle, (int *) &size);

      if (err < 0) {
        auto json = JSON::Object::Entries {
          {"source", "bufferSize"},
          {"err", JSON::Object::Entries {
            {"id", std::to_string(peerId)},
            {"code", "NOT_FOUND_ERR"},
            {"type", "NotFoundError"},
            {"message", String(uv_strerror(err))}
          }}
        };

        cb(seq, json, Post{});
        return;
      }

      auto json = JSON::Object::Entries {
        {"source", "bufferSize"},
        {"data", JSON::Object::Entries {
          {"id", std::to_string(peerId)},
          {"size", (int) size}
        }}
      };

      cb(seq, json, Post{});
    });
  }
}
