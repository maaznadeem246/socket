module;
#include "../core/utils.hh"

/**
 * @module ssc.utils
 * @description Various utility helper functions
 * @example
 * import ssc.util;
 * namespace ssc {
 *   auto u64 = utils::rand64();
 * }
 */
export module ssc.utils;
import ssc.types;
import ssc.uv;

using namespace ssc::types;

export namespace ssc::utils {
  const auto& rand64 = ssc::core::utils::rand64;

  using core::utils::decodeUTF8; // NOLINT(misc-unused-using-decls)
  using core::utils::decodeURIComponent; // NOLINT(misc-unused-using-decls)
  using core::utils::encodeURIComponent; // NOLINT(misc-unused-using-decls)

  inline String addrToIPv4 (struct sockaddr_in *sin) {
    char buf[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &sin->sin_addr, buf, INET_ADDRSTRLEN);
    return String(buf);
  }

  inline String addrToIPv6 (struct sockaddr_in6 *sin) {
    char buf[INET6_ADDRSTRLEN];
    inet_ntop(AF_INET6, &sin->sin6_addr, buf, INET6_ADDRSTRLEN);
    return String(buf);
  }

  inline void parseAddress (struct sockaddr *name, int* port, char* address) {
    struct sockaddr_in *name_in = (struct sockaddr_in *) name;
    *port = ntohs(name_in->sin_port);
    uv_ip4_name(name_in, address, 17);
  }
}
