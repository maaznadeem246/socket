module;
#include <socket/platform.hh>
#include "../core/bluetooth.hh"

export module ssc.bridge;
import ssc.ipc;
import ssc.json;
import ssc.dns;
import ssc.runtime;
import ssc.string;
import ssc.types;
import ssc.udp;
import ssc.utils;
import ssc.log;

using namespace ssc::core::bluetooth;
using namespace ssc::ipc;

using ssc::dns::DNS;
using ssc::platform::PlatformInfo;
using ssc::runtime::Runtime;
using ssc::udp::UDP;

#define resultCallback(message, reply)                                         \
  [=](auto seq, auto json, auto data) {                                        \
    reply(Result { seq, message, json, data });                                \
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

  class Bridge {
    public:
      Router router;
      Runtime& runtime;
      CoreBluetooth bluetooth;
      PlatformInfo platform;

      Bridge (const Bridge&) = delete;
      Bridge (Runtime& runtime)
        : runtime(runtime),
          router(runtime),
          bluetooth(router)
      {
        this->init();
      }

      inline void init () {
        /**
         * Starts a bluetooth service
         * @param serviceId
         */
        router.map("bluetooth.start", [&](auto message, auto _, auto reply) {
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
        router.map("bluetooth.subscribe", [&](auto message, auto _, auto reply) {
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
        router.map("bluetooth.publish", [&](auto message, auto _, auto reply) {
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

        /**
         * Maps a message buffer bytes to an index + sequence.
         *
         * This setup allows us to push a byte array to the bridge and
         * map it to an IPC call index and sequence pair, which is reused for an
         * actual IPC call, subsequently. This is used for systems that do not support
         * a POST/PUT body in XHR requests natively, so instead we decorate
         * `message.buffer` with already an mapped buffer.
         */
        router.map("buffer.map", false, [&](auto message, auto _, auto reply) {
          router.setMappedBuffer(
            message.index,
            message.seq,
            message.buffer.bytes,
            message.buffer.size
          );
        });

        /**
         * Returns pending data typically returned in the response of an
         * `ipc://data` IPC call intercepted by an XHR request.
         * @param id The id of the data.
         */
        router.map("data", [&](auto message, auto _, auto reply) {
          auto err = validateMessageParameters(message, {"id"});

          if (err.type != JSON::Type::Null) {
            return reply(Result::Err { message, err });
          }

          uint64_t id;
          getMessageParam(id, "id", std::stoull);

          if (!runtime.dataManager.has(id)) {
            return reply(Result::Err { message, JSON::Object::Entries {
              {"id", std::to_string(id)},
              {"message", "Data not found for given 'id'"}
            }});
          }

          auto result = Result { message.seq, message };
          result.value.data = runtime.dataManager.get(id);
          reply(result);
          runtime.dataManager.remove(id);
        });

        /**
         * Look up an IP address by `hostname`.
         * @param hostname Host name to lookup
         * @param family IP address family to resolve [default = 0 (AF_UNSPEC)]
         * @see getaddrinfo(3)
         */
        router.map("dns.lookup", [&](auto message, auto _, auto reply) {
          auto err = validateMessageParameters(message, {"hostname"});

          if (err.type != JSON::Type::Null) {
            return reply(Result::Err { message, err });
          }

          int family = 0;
          getMessageParam(family, "family", std::stoi, "0");

          runtime.dns.lookup(
            message.seq,
            DNS::LookupOptions { message.get("hostname"), family },
            resultCallback(message, reply)
          );
        });

       /**
        * Log `value to stdout` with platform dependent logger.
        * @param value
        */
        router.map("log", [&](auto message, auto _, auto reply) {
          log::info(message.value);
          reply(Result::Data { message, JSON::null });
        });

        /**
         * Read or modify the `SEND_BUFFER` or `RECV_BUFFER` for a peer socket.
         * @param id Handle ID for the buffer to read/modify
         * @param size If given, the size to set in the buffer [default = 0]
         * @param buffer The buffer to read/modify (SEND_BUFFER, RECV_BUFFER) [default = 0 (SEND_BUFFER)]
         */
        router.map("os.bufferSize", [&](auto message, auto _, auto reply) {
          auto err = validateMessageParameters(message, {"id"});

          if (err.type != JSON::Type::Null) {
            return reply(Result::Err { message, err });
          }

          uint64_t id; getMessageParam(id, "id", std::stoull);
          int buffer = 0; getMessageParam(buffer, "buffer", std::stoi, "0");
          int size = 0; getMessageParam(size, "size", std::stoi, "0");

          runtime.os.bufferSize(
            message.seq,
            id,
            size,
            buffer,
            resultCallback(message, reply)
          );
        });

       /**
        * Returns the platform OS.
        */
        router.map("os.platform", [&](auto message, auto _, auto reply) {
          auto result = Result { message.seq, message };
          result.value.json.data = platform.os;
          reply(result);
        });

        /**
         * Returns the platform type.
         */
        router.map("os.type", [&](auto message, auto _, auto reply) {
          auto result = Result { message.seq, message };
          result.value.json.data = platform.os;
          reply(result);
        });

        /**
         * Returns the platform architecture.
         */
        router.map("os.arch", [&](auto message, auto _, auto reply) {
          auto result = Result { message.seq, message };
          result.value.json.data = platform.arch;
          reply(result);
        });

        /**
         * Returns a mapping of network interfaces.
         */
        router.map("os.networkInterfaces", [&](auto message, auto _, auto reply) {
          runtime.os.networkInterfaces(message.seq, resultCallback(message, reply));
        });

        /**
         * Simply returns `pong`.
         */
        router.map("ping", [](auto message, auto _, auto reply) {
          auto result = Result { message.seq, message };
          result.value.json.data = "pong";
          reply(result);
        });

        /**
         * Handles platform events.
         * @param value The event name [domcontentloaded]
         * @param data Optional data associated with the platform event.
         */
        router.map("platform.event", [&](auto message, auto _, auto reply) {
          auto err = validateMessageParameters(message, {"value"});

          if (err.type != JSON::Type::Null) {
            return reply(Result::Err { message, err });
          }

          runtime.platform.event(
            message.seq,
            message.value,
            message.get("data"),
            resultCallback(message, reply)
          );
        });

        /**
         * Requests a notification with `title` and `body` to be shown.
         * @param title
         * @param body
         */
        router.map("platform.notify", [&](auto message, auto _, auto reply) {
          auto err = validateMessageParameters(message, {"body", "title"});

          if (err.type != JSON::Type::Null) {
            return reply(Result::Err { message, err });
          }

          runtime.platform.notify(
            message.seq,
            message.get("title"),
            message.get("body"),
            resultCallback(message, reply)
          );
        });

        /**
         * Requests a URL to be opened externally.
         * @param value
         */
        router.map("platform.openExternal", [&](auto message, auto _, auto reply) {
          auto err = validateMessageParameters(message, {"value"});

          if (err.type != JSON::Type::Null) {
            return reply(Result::Err { message, err });
          }

          runtime.platform.openExternal(
            message.seq,
            message.value,
            resultCallback(message, reply)
          );
        });

        /**
         * Returns computed current working directory path.
         */
        router.map("process.cwd", [&](auto message, auto router, auto reply) {
          runtime.platform.cwd(message.seq, resultCallback(message, reply));
        });

        /**
         * Prints incoming message value to stdout.
         */
        router.map("stdout", [&](auto message, auto router, auto reply) {
          log::write(message.value, false);
        });

       /**
        * Prints incoming message value to stderr.
        */
        router.map("stderr", [&](auto message, auto router, auto reply) {
          log::write(message.value, true);
        });

        /**
         * Binds an UDP socket to a specified port, and optionally a host
         * address (default: 0.0.0.0).
         * @param id Handle ID of underlying socket
         * @param port Port to bind the UDP socket to
         * @param address The address to bind the UDP socket to (default: 0.0.0.0)
         * @param reuseAddr Reuse underlying UDP socket address (default: false)
         */
        router.map("udp.bind", [&](auto message, auto router, auto reply) {
          UDP::BindOptions options;
          auto err = validateMessageParameters(message, {"id", "port"});

          if (err.type != JSON::Type::Null) {
            return reply(Result::Err { message, err });
          }

          uint64_t id;
          getMessageParam(id, "id", std::stoull);
          getMessageParam(options.port, "port", std::stoi);

          options.reuseAddr = message.get("reuseAddr") == "true";
          options.address = message.get("address", "0.0.0.0");

          runtime.udp.bind(
            message.seq,
            id,
            options,
            resultCallback(message, reply)
          );
        });

        /**
         * Close socket handle and underlying UDP socket.
         * @param id Handle ID of underlying socket
         */
        router.map("udp.close", [&](auto message, auto router, auto reply) {
          auto err = validateMessageParameters(message, {"id"});

          if (err.type != JSON::Type::Null) {
            return reply(Result::Err { message, err });
          }

          uint64_t id;
          getMessageParam(id, "id", std::stoull);

          runtime.udp.close(message.seq, id, resultCallback(message, reply));
        });

        /**
         * Connects an UDP socket to a specified port, and optionally a host
         * address (default: 0.0.0.0).
         * @param id Handle ID of underlying socket
         * @param port Port to connect the UDP socket to
         * @param address The address to connect the UDP socket to (default: 0.0.0.0)
         */
        router.map("udp.connect", [&](auto message, auto router, auto reply) {
          auto err = validateMessageParameters(message, {"id", "port"});

          if (err.type != JSON::Type::Null) {
            return reply(Result::Err { message, err });
          }

          UDP::ConnectOptions options;
          uint64_t id;
          getMessageParam(id, "id", std::stoull);
          getMessageParam(options.port, "port", std::stoi);

          options.address = message.get("address", "0.0.0.0");

          runtime.udp.connect(
            message.seq,
            id,
            options,
            resultCallback(message, reply)
          );
        });

       /**
        * Disconnects a connected socket handle and underlying UDP socket.
        * @param id Handle ID of underlying socket
        */
        router.map("udp.disconnect", [&](auto message, auto router, auto reply) {
          auto err = validateMessageParameters(message, {"id"});

          if (err.type != JSON::Type::Null) {
            return reply(Result::Err { message, err });
          }

          uint64_t id;
          getMessageParam(id, "id", std::stoull);

          runtime.udp.disconnect(
            message.seq,
            id,
            resultCallback(message, reply)
          );
        });

        /**
         * Returns connected peer socket address information.
         * @param id Handle ID of underlying socket
         */
        router.map("udp.getPeerName", [&](auto message, auto router, auto reply) {
          auto err = validateMessageParameters(message, {"id"});

          if (err.type != JSON::Type::Null) {
            return reply(Result::Err { message, err });
          }

          uint64_t id;
          getMessageParam(id, "id", std::stoull);

          runtime.udp.getPeerName(
            message.seq,
            id,
            resultCallback(message, reply)
          );
        });

        /**
         * Returns local socket address information.
         * @param id Handle ID of underlying socket
         */
        router.map("udp.getSockName", [&](auto message, auto router, auto reply) {
          auto err = validateMessageParameters(message, {"id"});

          if (err.type != JSON::Type::Null) {
            return reply(Result::Err { message, err });
          }

          uint64_t id;
          getMessageParam(id, "id", std::stoull);

          runtime.udp.getSockName(
            message.seq,
            id,
            resultCallback(message, reply)
          );
        });

        /**
         * Returns socket state information.
         * @param id Handle ID of underlying socket
         */
        router.map("udp.getState", [&](auto message, auto router, auto reply) {
          auto err = validateMessageParameters(message, {"id"});

          if (err.type != JSON::Type::Null) {
            return reply(Result::Err { message, err });
          }

          uint64_t id;
          getMessageParam(id, "id", std::stoull);

          runtime.udp.getState(
            message.seq,
            id,
            resultCallback(message, reply)
          );
        });

        /**
         * Initializes socket handle to start receiving data from the underlying
         * socket and route through the IPC bridge to the WebView.
         * @param id Handle ID of underlying socket
         */
        router.map("udp.readStart", [&](auto message, auto router, auto reply) {
          auto err = validateMessageParameters(message, {"id"});

          if (err.type != JSON::Type::Null) {
            return reply(Result::Err { message, err });
          }

          uint64_t id;
          getMessageParam(id, "id", std::stoull);

          runtime.udp.readStart(
            message.seq,
            id,
            resultCallback(message, reply)
          );
        });

        /**
         * Stops socket handle from receiving data from the underlying
         * socket and routing through the IPC bridge to the WebView.
         * @param id Handle ID of underlying socket
         */
        router.map("udp.readStop", [&](auto message, auto router, auto reply) {
          auto err = validateMessageParameters(message, {"id"});

          if (err.type != JSON::Type::Null) {
            return reply(Result::Err { message, err });
          }

          uint64_t id;
          getMessageParam(id, "id", std::stoull);

          runtime.udp.readStop(
            message.seq,
            id,
            resultCallback(message, reply)
          );
        });

        /**
         * Broadcasts a datagram on the socket. For connectionless sockets, the
         * destination port and address must be specified. Connected sockets, on the
         * other hand, will use their associated remote endpoint, so the port and
         * address arguments must not be set.
         * @param id Handle ID of underlying socket
         * @param port The port to send data to
         * @param size The size of the bytes to send
         * @param bytes A pointer to the bytes to send
         * @param address The address to send to (default: 0.0.0.0)
         * @param ephemeral Indicates that the socket handle, if created is ephemeral and should eventually be destroyed
         */
        router.map("udp.send", [&](auto message, auto router, auto reply) {
          auto err = validateMessageParameters(message, {"id", "port"});

          if (err.type != JSON::Type::Null) {
            return reply(Result::Err { message, err });
          }

          UDP::SendOptions options;
          uint64_t id;
          getMessageParam(id, "id", std::stoull);
          getMessageParam(options.port, "port", std::stoi);

          options.size = message.buffer.size;
          options.bytes = message.buffer.bytes;
          options.address = message.get("address", "0.0.0.0");
          options.ephemeral = message.get("ephemeral") == "true";

          runtime.udp.send(
            message.seq,
            id,
            options,
            resultCallback(message, reply)
          );
        });
      }
  };
}
