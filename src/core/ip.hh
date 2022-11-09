#ifndef SSC_CORE_IP_HH
#define SSC_CORE_IP_HH

#if !defined(SSC_INLINE_INCLUDE)
#include "platform.hh"
#include "string.hh"
#endif

#if !defined(SSC_INLINE_INCLUDE)
namespace ssc::ip {
  using namespace ssc::string;
#endif
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
#if !defined(SSC_INLINE_INCLUDE)
}
#endif
#endif
