#include "routing.hh"
#include "ipc.hh"

using namespace ssc::core::ipc;
using namespace ssc::core::routing;

#define resultCallback(message, reply)                                         \
  [=](auto seq, auto json, auto post) {                                        \
    reply(Result { seq, message, json, post });                                \
  }

#define getMessageParam(var, name, parse, ...)                                 \
  try {                                                                        \
    var = parse(message.get(name, ##__VA_ARGS__));                             \
  } catch (...) {                                                              \
    return reply(Result::Err { message, JSON::Object::Entries {                \
      {"message", "Invalid '" name "' given in parameters"}                    \
    }});                                                                       \
  }

#if defined(__APPLE__)
  static dispatch_queue_attr_t qos = dispatch_queue_attr_make_with_qos_class(
    DISPATCH_QUEUE_CONCURRENT,
    QOS_CLASS_USER_INITIATED,
    -1
  );

  static dispatch_queue_t queue = dispatch_queue_create(
    "co.socketsupply.queue.routing",
    qos
  );
#endif

namespace ssc::core::routing {
  JSON::Any validateMessageParameters (
    const Message& message,
    const Vector<String> names
  ) {
    for (const auto& name : names) {
      if (!message.has(name) || message.get(name).size() == 0) {
        return JSON::Object::Entries {
          {"message", "Expecting '" + name + "' in parameters"}
        };
      }
    }

    return nullptr;
  }

  void init (ipc::IRouter& router) {
    /**
     * Starts a bluetooth service
     * @param serviceId
     */
    router.map("bluetooth.start", [](auto message, auto router, auto reply) {
      auto err = validateMessageParameters(message, {"serviceId"});

      if (err.type != JSON::Type::Null) {
        return reply(Result::Err { message, err });
      }

      //router.bridge->bluetooth.startService(
        //message.seq,
        //message.get("serviceId")
      //);
    });

    /**
     * Subscribes to a characteristic for a service.
     * @param serviceId
     * @param characteristicId
     */
    router.map("bluetooth.subscribe", [](auto message, auto router, auto reply) {
      auto err = validateMessageParameters(message, {
        "characteristicId",
        "serviceId"
      });

      if (err.type != JSON::Type::Null) {
        return reply(Result::Err { message, err });
      }

      //router.bridge->bluetooth.subscribeCharacteristic(
        //message.seq,
        //message.get("serviceId"),
        //message.get("characteristicId")
      //);
    });

    /**
     * Publishes data to a characteristic for a service.
     * @param serviceId
     * @param characteristicId
     */
    router.map("bluetooth.publish", [](auto message, auto router, auto reply) {
      auto err = validateMessageParameters(message, {
        "characteristicId",
        "serviceId"
      });

      if (err.type != JSON::Type::Null) {
        return reply(Result::Err { message, err });
      }

      auto bytes = message.buffer.bytes;
      auto size = message.buffer.size;

      if (bytes == nullptr) {
        bytes = message.value.data();
        size = message.value.size();
      }

      //router.bridge->bluetooth.publishCharacteristic(
        //message.seq,
        //bytes,
        //size,
        //message.get("serviceId"),
        //message.get("characteristicId")
      //);
    });
  }
}
