module;
#include "../core/bluetooth.hh"

export module ssc.bridge;
import ssc.ipc;
import ssc.json;
import ssc.runtime;
import ssc.string;
import ssc.types;
import ssc.utils;
import ssc.log;

using namespace ssc::core::bluetooth;
using namespace ssc::ipc;
using ssc::runtime::Runtime;

#define IPC_CONTENT_TYPE "application/octet-stream"

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
    "co.socketsupply.queue.bridge",
    qos
  );
#endif

export namespace ssc::bridge {
  // forward

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

  inline void routing (Router& router) {
    CoreBluetooth bluetooth(router);

    /**
     * Starts a bluetooth service
     * @param serviceId
     */
    router.map("bluetooth.start", [&](auto message, auto router, auto reply) {
      auto err = validateMessageParameters(message, {"serviceId"});

      if (err.type != JSON::Type::Null) {
        return reply(Result::Err { message, err });
      }

      bluetooth.startService(
        message.seq,
        message.get("serviceId")
      );
    });

    /**
     * Subscribes to a characteristic for a service.
     * @param serviceId
     * @param characteristicId
     */
    router.map("bluetooth.subscribe", [&](auto message, auto router, auto reply) {
      auto err = validateMessageParameters(message, {
        "characteristicId",
        "serviceId"
      });

      if (err.type != JSON::Type::Null) {
        return reply(Result::Err { message, err });
      }

      bluetooth.subscribeCharacteristic(
        message.seq,
        message.get("serviceId"),
        message.get("characteristicId")
      );
    });

    /**
     * Publishes data to a characteristic for a service.
     * @param serviceId
     * @param characteristicId
     */
    router.map("bluetooth.publish", [&](auto message, auto router, auto reply) {
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

      bluetooth.publishCharacteristic(
        message.seq,
        bytes,
        size,
        message.get("serviceId"),
        message.get("characteristicId")
      );
    });
  }

  class Bridge {
    public:
      Router router;
      Runtime& runtime;
      Bridge (const Bridge&) = delete;
      Bridge (Runtime& runtime)
        : runtime(runtime),
          router(runtime)
      {
        routing(this->router);
      }
  };
}
