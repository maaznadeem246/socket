module;
#include <socket/platform.hh>

/**
 * @module ssc.ip
 * @description TODO
 * @example
 * TODO
 */
export module ssc.ip;
import ssc.string;
using String = ssc::string::String;
export namespace ssc::ip {
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
}
