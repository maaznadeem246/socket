#include <socket/runtime.hh>

namespace ssc::runtime {
  DNS::DNS (Runtime* runtime)
    : loop(runtime->loops.main)
  {}

  void DNS::lookup (const String seq, LookupOptions options, Callback callback) {
    this->loop.dispatch([=, this]() {
      auto ctx = new RequestContext(seq, callback);
      auto loop = this->loop.get();

      struct addrinfo hints = {0};

      if (options.family == 6) {
        hints.ai_family = AF_INET6;
      } else if (options.family == 4) {
        hints.ai_family = AF_INET;
      } else {
        hints.ai_family = AF_UNSPEC;
      }

      hints.ai_socktype = 0; // `0` for any
      hints.ai_protocol = 0; // `0` for any

      auto resolver = new uv_getaddrinfo_t;
      resolver->data = ctx;
      ctx->options = options;

      auto err = uv_getaddrinfo(loop, resolver, [](uv_getaddrinfo_t *resolver, int status, struct addrinfo *res) {
        auto ctx = (DNS::RequestContext*) resolver->data;
        if (status < 0) {
          auto result = JSON::Object::Entries {
            {"source", "dns.lookup"},
            {"err", JSON::Object::Entries {
              {"code", std::to_string(status)},
              {"message", String(uv_strerror(status))}
            }}
          };

          ctx->callback(ctx->seq, result, Data{});
          delete ctx;
          return;
        }

        String address = "";

        // @TODO(jwerle): support `options.all`
        // loop through linked list of results

        if (res->ai_family == AF_INET) {
          char addr[17] = {'\0'};
          uv_ip4_name((struct sockaddr_in*)(res->ai_addr), addr, 16);
          address = String(addr, 17);
        } else if (res->ai_family == AF_INET6) {
          char addr[40] = {'\0'};
          uv_ip6_name((struct sockaddr_in6*)(res->ai_addr), addr, 39);
          address = String(addr, 40);
        }

        address = address.erase(address.find('\0'));

        auto family = res->ai_family == AF_INET
          ? 4
          : res->ai_family == AF_INET6
            ? 6
            : 0;

        auto result = JSON::Object::Entries {
          {"source", "dns.lookup"},
          {"data", JSON::Object::Entries {
            {"address", address},
            {"family", family}
          }}
        };

        ctx->callback(ctx->seq, result, Data{});
        uv_freeaddrinfo(res);
        delete ctx;

      }, options.hostname.c_str(), nullptr, &hints);

      if (err < 0) {
        auto result = JSON::Object::Entries {
          {"source", "dns.lookup"},
          {"err", JSON::Object::Entries {
            {"code", std::to_string(err)},
            {"message", String(uv_strerror(err))}
          }}
        };

        ctx->callback(seq, result, Data{});
        delete ctx;
      }
    });
  }

  void DNS::lookup (LookupOptions options, Callback callback) {
    return this->lookup("-1", options, callback);
  }
}
