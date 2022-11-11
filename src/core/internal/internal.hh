#ifndef SSC_CORE_INTERNAL_HH
#define SSC_CORE_INTERNAL_HH

#if !defined(SSC_INLINE_INCLUDE)
  #include <socket/platform.hh>
  #include <socket/common.hh>
  #include <any>
  #include <map>
  #include <mutex>
  #include <queue>
  #include <regex>
  #include <semaphore>
  #include <string>
  #include <sstream>
  #include <thread>
  #include <vector>
#endif

/**
 * Internal namespace for out-of-module usage
 */
namespace ssc::internal {
  #define SSC_INLINE_INCLUDE
  namespace types {
    #include "../types.hh"
  }

  namespace string {
    using namespace types;
    #include "../string.hh"
  }

  namespace utils{
    #include "../utils.hh"
  }

  namespace JSON {
    #include "../json.hh"
  }

  namespace javascript {
    using namespace string;
    #include "../javascript.hh"
  }

  namespace codec {
    using namespace string;
    #include "../codec.hh"
  }

  namespace headers {
    using namespace string;
    #include "../headers.hh"
  }

  namespace ipc {
    namespace data {
      using namespace headers;
      using namespace utils;
      #include "../ipc/data.hh"
    }

    namespace message {
      using namespace string;
      using namespace codec;
      #include "../ipc/message.hh"
    }

    namespace result {
      using namespace string;
      using namespace codec;
      #include "../ipc/result.hh"
    }
  }
  #undef SSC_INLINE_INCLUDE
}
#endif
