module; // global
#include "../platform.hh"
#include <string>
#include <stdio.h>

/**
 * @module ssc.dns
 * @description DNS runtime API
 * @example
 * import ssc.dns
 * namespace ssc {
 *   using DNS = ssc::dns::DNS:
 *   DNS dns(runtime);
 *   dns.lookup(LookupOptions { "sockets.sh", 4 }, [](auto seq, auto json, auto data) {
 *     printf("address=%s\n", json.get("data").as<JSON::Object>().get("address").str().c_str());
 *   });
 * }
 * TODO
 */
export module ssc.dns;
import ssc.runtime;
import ssc.context;
import ssc.string;
import ssc.types;
import ssc.json;
import ssc.uv;

using ssc::context::Context;
using ssc::runtime::Runtime;
using ssc::string::String;
using ssc::types::Post;

export namespace ssc::dns {
  class DNS : public Context {
    public:
      DNS (Runtime* runtime) : Context(runtime) {}

      struct LookupOptions {
        String hostname;
        int family = 4;
        bool all = false;
        // TODO: support these options
        // - hints
        // -verbatim
      };

      struct RequestContext : Context::RequestContext {
        LookupOptions options;
        RequestContext (const String& seq, Callback cb)
          : Context::RequestContext(seq, cb) {}
      };

      void lookup (const String seq, LookupOptions options, Callback cb) {
        this->runtime->dispatch([=, this]() {
          auto ctx = new RequestContext(seq, cb);
          auto loop = this->runtime->loop.get();

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

              ctx->cb(ctx->seq, result, Post{});
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

      void lookup (LookupOptions options, Callback cb) {
        return this->lookup("-1", options, cb);
      }
  };
}
