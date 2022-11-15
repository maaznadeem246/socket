#ifndef SSC_CORE_ROUTING_HH
#define SSC_CORE_ROUTING_HH

#include <socket/platform.hh>
#include "types.hh"
#include "ipc.hh"

namespace ssc::core::routing {
  #define IPC_CONTENT_TYPE "application/octet-stream"

  void init (ipc::IRouter& router);
}
#endif
