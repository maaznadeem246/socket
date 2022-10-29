/**
 * Global module fragment
 */

module;
#include "../platform.hh"

/**
 * `ssc.runtime:dns` module fragment.
 */
export module dns;
import runtime;
import context;
import json;

export namespace ssc {
  class DNS : public Context {
    public:
      DNS (Runtime* runtime) : Context(runtime) {}
      struct LookupOptions {
        String hostname;
        int family;
        // TODO: support these options
        // - hints
        // - all
        // -verbatim
      };

      void lookup (const String seq, LookupOptions options, Callback cb) {
        this->runtime->dispatchEventLoop([=, this]() {
          auto ctx = new Context::RequestContext(seq, cb);
          auto loop = this->runtime->getEventLoop();

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

          uv_getaddrinfo_t* resolver = new uv_getaddrinfo_t;
          resolver->data = ctx;

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

              ctx->cb(ctx->seq, result, Post{});
              delete ctx;
              return;
            }

            String address = "";

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

            ctx->cb(ctx->seq, result, Post{});
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

            ctx->cb(seq, result, Post{});
            delete ctx;
          }
        });
      }
  };
}
