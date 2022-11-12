module;

#include "../platform.hh"
#include <uv.h>

export module udp;
import context;
import json;
import peer;

export namespace ssc {
  class Runtime;
  void parseAddress (struct sockaddr *name, int* port, char* address) {
    struct sockaddr_in *name_in = (struct sockaddr_in *) name;
    *port = ntohs(name_in->sin_port);
    uv_ip4_name(name_in, address, 17);
  }

  static JSON::Object::Entries ERR_SOCKET_ALREADY_BOUND (
    const String& source,
    uint64_t id
  ) {
    return JSON::Object::Entries {
      {"source", source},
      {"err", JSON::Object::Entries {
        {"id", std::to_string(id)},
        {"type", "InternalError"},
        {"code", "ERR_SOCKET_ALREADY_BOUND"},
        {"message", "Socket is already bound"}
      }}
    };
  }

  static JSON::Object::Entries ERR_SOCKET_DGRAM_IS_CONNECTED (
    const String &source,
    uint64_t id
  ) {
    return JSON::Object::Entries {
      {"source", source},
      {"err", JSON::Object::Entries {
        {"id", std::to_string(id)},
        {"type", "InternalError"},
        {"code", "ERR_SOCKET_DGRAM_IS_CONNECTED"},
        {"message", "Already connected"}
      }}
    };
  }

  static JSON::Object::Entries ERR_SOCKET_DGRAM_NOT_CONNECTED (
    const String &source,
    uint64_t id
  ) {
    return JSON::Object::Entries {
      {"source", source},
      {"err", JSON::Object::Entries {
        {"id", std::to_string(id)},
        {"type", "InternalError"},
        {"code", "ERR_SOCKET_DGRAM_NOT_CONNECTED"},
        {"message", "Not connected"}
      }}
    };
  }

  static JSON::Object::Entries ERR_SOCKET_DGRAM_CLOSED (
    const String& source,
    uint64_t id
  ) {
    return JSON::Object::Entries {
      {"source", source},
      {"err", JSON::Object::Entries {
        {"id", std::to_string(id)},
        {"type", "NotFoundError"},
        {"code", "ERR_SOCKET_DGRAM_CLOSED"},
        {"message", "Socket is closed"}
      }}
    };
  }

  static JSON::Object::Entries ERR_SOCKET_DGRAM_CLOSING (
    const String& source,
    uint64_t id
  ) {
    return JSON::Object::Entries {
      {"source", source},
      {"err", JSON::Object::Entries {
        {"id", std::to_string(id)},
        {"type", "NotFoundError"},
        {"code", "ERR_SOCKET_DGRAM_CLOSING"},
        {"message", "Socket is closing"}
      }}
    };
  }

  static JSON::Object::Entries ERR_SOCKET_DGRAM_NOT_RUNNING (
    const String& source,
    uint64_t id
  ) {
    return JSON::Object::Entries {
      {"source", source},
      {"err", JSON::Object::Entries {
        {"id", std::to_string(id)},
        {"type", "NotFoundError"},
        {"code", "ERR_SOCKET_DGRAM_NOT_RUNNING"},
        {"message", "Not running"}
      }}
    };
  }

  class UDP : public Context {
    public:
      UDP (auto core) : Context(core) {}

      struct BindOptions {
        String address;
        int port;
        bool reuseAddr = false;
      };

      struct ConnectOptions {
        String address;
        int port;
      };

      struct SendOptions {
        String address = "";
        int port = 0;
        char *bytes = nullptr;
        size_t size = 0;
        bool ephemeral = false;
      };

      void bind (
        const String seq,
        uint64_t id,
        BindOptions options,
        Context::Callback cb
      );
      void close (const String seq, uint64_t id, Context::Callback cb);
      void connect (
        const String seq,
        uint64_t id,
        ConnectOptions options,
        Context::Callback cb
      );
      void disconnect (const String seq, uint64_t id, Context::Callback cb);
      void getPeerName (const String seq, uint64_t id, Context::Callback cb);
      void getSockName (const String seq, uint64_t id, Context::Callback cb);
      void getState (const String seq, uint64_t id,  Context::Callback cb);
      void readStart (const String seq, uint64_t id, Context::Callback cb);
      void readStop (const String seq, uint64_t id, Context::Callback cb);
      void send (
        const String seq,
        uint64_t id,
        SendOptions options,
        Context::Callback cb
      );
  };

  void UDP::bind (
    const String seq,
    uint64_t peerId,
    UDP::BindOptions options,
    Context::Callback cb
  ) {
    this->runtime->dispatchEventLoop([=, this]() {
      if (this->runtime->hasPeer(peerId)) {
        if (this->runtime->getPeer(peerId)->isBound()) {
          auto json = ERR_SOCKET_ALREADY_BOUND("udp.bind", peerId);
          return cb(seq, json, Post{});
        }
      }

      auto peer = this->runtime->createPeer(PEER_TYPE_UDP, peerId);
      auto err = peer->bind(options.address, options.port, options.reuseAddr);

      if (err < 0) {
        auto json = JSON::Object::Entries {
          {"source", "udp.bind"},
          {"err", JSON::Object::Entries {
            {"id", std::to_string(peerId)},
            {"message", String(uv_strerror(err))}
          }}
        };

        return cb(seq, json, Post{});
      }

      auto info = peer->getLocalPeerInfo();

      if (info->err < 0) {
        auto json = JSON::Object::Entries {
          {"source", "udp.bind"},
          {"err", JSON::Object::Entries {
            {"id", std::to_string(peerId)},
            {"message", String(uv_strerror(info->err))}
          }}
        };

        return cb(seq, json, Post{});
      }

      auto json = JSON::Object::Entries {
        {"source", "udp.bind"},
        {"data", JSON::Object::Entries {
          {"id", std::to_string(peerId)},
          {"port", (int) info->port},
          {"event" , "listening"},
          {"family", info->family},
          {"address", info->address}
        }}
      };

      cb(seq, json, Post{});
    });
  }

  void UDP::connect (
    const String seq,
    uint64_t peerId,
    UDP::ConnectOptions options,
    Context::Callback cb
  ) {
    this->runtime->dispatchEventLoop([=, this]() {
      auto peer = this->runtime->createPeer(PEER_TYPE_UDP, peerId);

      if (peer->isConnected()) {
        auto json = ERR_SOCKET_DGRAM_IS_CONNECTED("udp.connect", peerId);
        return cb(seq, json, Post{});
      }

      auto err = peer->connect(options.address, options.port);

      if (err < 0) {
        auto json = JSON::Object::Entries {
          {"source", "udp.connect"},
          {"err", JSON::Object::Entries {
            {"id", std::to_string(peerId)},
            {"message", String(uv_strerror(err))}
          }}
        };

        return cb(seq, json, Post{});
      }

      auto info = peer->getRemotePeerInfo();

      if (info->err < 0) {
        auto json = JSON::Object::Entries {
          {"source", "udp.connect"},
          {"err", JSON::Object::Entries {
            {"id", std::to_string(peerId)},
            {"message", String(uv_strerror(info->err))}
          }}
        };

        return cb(seq, json, Post{});
      }

      auto json = JSON::Object::Entries {
        {"source", "udp.connect"},
        {"data", JSON::Object::Entries {
          {"address", info->address},
          {"family", info->family},
          {"port", (int) info->port},
          {"id", std::to_string(peerId)}
        }}
      };

      cb(seq, json, Post{});
    });
  }

  void UDP::disconnect (
    const String seq,
    uint64_t peerId,
    Context::Callback cb
  ) {
    this->runtime->dispatchEventLoop([=, this]() {
      if (!this->runtime->hasPeer(peerId)) {
        auto json = ERR_SOCKET_DGRAM_NOT_CONNECTED("udp.disconnect", peerId);
        return cb(seq, json, Post{});
      }

      auto peer = this->runtime->getPeer(peerId);
      auto err = peer->disconnect();

      if (err < 0) {
        auto json = JSON::Object::Entries {
          {"source", "udp.disconnect"},
          {"err", JSON::Object::Entries {
            {"id", std::to_string(peerId)},
            {"message", String(uv_strerror(err))}
          }}
        };

        return cb(seq, json, Post{});
      }

      auto json = JSON::Object::Entries {
        {"source", "udp.disconnect"},
        {"data", JSON::Object::Entries {
          {"id", std::to_string(peerId)}
        }}
      };

      cb(seq, json, Post{});
    });
  }

  void UDP::getPeerName (String seq, uint64_t peerId, Context::Callback cb) {
    if (!this->runtime->hasPeer(peerId)) {
      auto json = ERR_SOCKET_DGRAM_NOT_CONNECTED("udp.getPeerName", peerId);
      return cb(seq, json, Post{});
    }

    auto peer = this->runtime->getPeer(peerId);
    auto info = peer->getRemotePeerInfo();

    if (info->err < 0) {
      auto json = JSON::Object::Entries {
        {"source", "udp.getPeerName"},
        {"err", JSON::Object::Entries {
          {"id", std::to_string(peerId)},
          {"message", String(uv_strerror(info->err))}
        }}
      };

      return cb(seq, json, Post{});
    }

    auto json = JSON::Object::Entries {
      {"source", "udp.getPeerName"},
      {"data", JSON::Object::Entries {
        {"address", info->address},
        {"family", info->family},
        {"port", (int) info->port},
        {"id", std::to_string(peerId)}
      }}
    };

    cb(seq, json, Post{});
  }

  void UDP::getSockName (String seq, uint64_t peerId, Callback cb) {
    if (!this->runtime->hasPeer(peerId)) {
      auto json = ERR_SOCKET_DGRAM_NOT_RUNNING("udp.getSockName", peerId);
      return cb(seq, json, Post{});
    }

    auto peer = this->runtime->getPeer(peerId);
    auto info = peer->getLocalPeerInfo();

    if (info->err < 0) {
      auto json = JSON::Object::Entries {
        {"source", "udp.getSockName"},
        {"err", JSON::Object::Entries {
          {"id", std::to_string(peerId)},
          {"message", String(uv_strerror(info->err))}
        }}
      };

      return cb(seq, json, Post{});
    }

    auto json = JSON::Object::Entries {
      {"source", "udp.getSockName"},
      {"data", JSON::Object::Entries {
        {"address", info->address},
        {"family", info->family},
        {"port", (int) info->port},
        {"id", std::to_string(peerId)}
      }}
    };

    cb(seq, json, Post{});
  }

  void UDP::getState (
    const String seq,
    uint64_t peerId,
    Context::Callback cb
  ) {
    if (!this->runtime->hasPeer(peerId)) {
      auto json = ERR_SOCKET_DGRAM_NOT_RUNNING("udp.getState", peerId);
      return cb(seq, json, Post{});
    }

    auto peer = this->runtime->getPeer(peerId);

    if (!peer->isUDP()) {
      auto json = ERR_SOCKET_DGRAM_NOT_RUNNING("udp.getState", peerId);
      return cb(seq, json, Post{});
    }

    auto json = JSON::Object::Entries {
      {"source", "udp.getState"},
      {"data", JSON::Object::Entries {
        {"id", std::to_string(peerId)},
        {"type", "udp"},
        {"bound", peer->isBound()},
        {"active", peer->isActive()},
        {"closed", peer->isClosed()},
        {"closing", peer->isClosing()},
        {"connected", peer->isConnected()},
        {"ephemeral", peer->isEphemeral()}
      }}
    };

    cb(seq, json, Post{});
  }

  void UDP::send (
    String seq,
    uint64_t peerId,
    UDP::SendOptions options,
    Context::Callback cb
  ) {
    this->runtime->dispatchEventLoop([=, this] {
      auto peer = this->runtime->createPeer(PEER_TYPE_UDP, peerId, options.ephemeral);
      auto size = options.size; // @TODO(jwerle): validate MTU
      auto port = options.port;
      auto bytes = options.bytes;
      auto address = options.address;
      peer->send(bytes, size, port, address, [=](auto status, auto post) {
        if (status < 0) {
          auto json = JSON::Object::Entries {
            {"source", "udp.send"},
            {"err", JSON::Object::Entries {
              {"id", std::to_string(peerId)},
              {"message", String(uv_strerror(status))}
            }}
          };

          return cb(seq, json, Post{});
        }

        auto json = JSON::Object::Entries {
          {"source", "udp.send"},
          {"data", JSON::Object::Entries {
            {"id", std::to_string(peerId)},
            {"status", status}
          }}
        };

        cb(seq, json, Post{});
      });
    });
  }

  void UDP::readStart (String seq, uint64_t peerId, Context::Callback cb) {
    if (!this->runtime->hasPeer(peerId)) {
      auto json = ERR_SOCKET_DGRAM_NOT_RUNNING("udp.readStart", peerId);
      return cb(seq, json, Post{});
    }

    auto peer = this->runtime->getPeer(peerId);

    if (peer->isClosed()) {
      auto json = ERR_SOCKET_DGRAM_CLOSED("udp.readStart", peerId);
      return cb(seq, json, Post{});
    }

    if (peer->isClosing()) {
      auto json = ERR_SOCKET_DGRAM_CLOSING("udp.readStart", peerId);
      return cb(seq, json, Post{});
    }

    if (peer->hasState(PEER_STATE_UDP_RECV_STARTED)) {
      auto json = JSON::Object::Entries {
        {"source", "udp.readStart"},
        {"err", JSON::Object::Entries {
          {"id", std::to_string(peerId)},
          {"message", "Socket is already receiving"}
        }}
      };

      return cb(seq, json, Post{});
    }

    if (peer->isActive()) {
      auto json = JSON::Object::Entries {
        {"source", "udp.readStart"},
        {"data", JSON::Object::Entries {
          {"id", std::to_string(peerId)}
        }}
      };

      return cb(seq, json, Post{});
    }

    auto err = peer->recvstart([=](auto nread, auto buf, auto addr) {
      if (nread == UV_EOF) {
        auto json = JSON::Object::Entries {
          {"source", "udp.readStart"},
          {"data", JSON::Object::Entries {
            {"id", std::to_string(peerId)},
            {"EOF", true}
          }}
        };

        return cb("-1", json, Post{});
      }

      if (nread > 0) {
        char address[17];
        Post post;
        int port;

        parseAddress((struct sockaddr *) addr, &port, address);

        auto headers = Headers {{
          {"content-type" ,"application/octet-stream"},
          {"content-length", nread}
        }};

        post.id = rand64();
        post.body = buf->base;
        post.length = (int) nread;
        post.headers = headers.str();
        post.bodyNeedsFree = true;

        auto json = JSON::Object::Entries {
          {"source", "udp.readStart"},
          {"data", JSON::Object::Entries {
            {"id", std::to_string(peerId)},
            {"port", port},
            {"bytes", std::to_string(post.length)},
            {"address", address}
          }}
        };

        return cb("-1", json, post);
      }
    });

    // `UV_EALREADY || UV_EBUSY` could mean there might be
    // active IO on the underlying handle
    if (err < 0 && err != UV_EALREADY && err != UV_EBUSY) {
      auto json = JSON::Object::Entries {
        {"source", "udp.readStart"},
        {"err", JSON::Object::Entries {
          {"id", std::to_string(peerId)},
          {"message", String(uv_strerror(err))}
        }}
      };

      return cb(seq, json, Post{});
    }

    auto json = JSON::Object::Entries {
      {"source", "udp.readStart"},
      {"data", JSON::Object::Entries {
        {"id", std::to_string(peerId)}
      }}
    };

    cb(seq, json, Post {});
  }

  void UDP::readStop (
    const String seq,
    uint64_t peerId,
    Context::Callback cb
  ) {
    this->runtime->dispatchEventLoop([=, this] {
      if (!this->runtime->hasPeer(peerId)) {
        auto json = ERR_SOCKET_DGRAM_NOT_RUNNING("udp.readStop", peerId);
        return cb(seq, json, Post{});
      }

      auto peer = this->runtime->getPeer(peerId);

      if (peer->isClosed()) {
        auto json = ERR_SOCKET_DGRAM_CLOSED("udp.readStop", peerId);
        return cb(seq, json, Post{});
      }

      if (peer->isClosing()) {
        auto json = ERR_SOCKET_DGRAM_CLOSING("udp.readStop", peerId);
        return cb(seq, json, Post{});
      }

      if (!peer->hasState(PEER_STATE_UDP_RECV_STARTED)) {
        auto json = JSON::Object::Entries {
          {"source", "udp.readStop"},
          {"err", JSON::Object::Entries {
            {"id", std::to_string(peerId)},
            {"message", "Socket is not receiving"}
          }}
        };

        return cb(seq, json, Post{});
      }

      auto err = peer->recvstop();

      if (err < 0) {
        auto json = JSON::Object::Entries {
          {"source", "udp.readStop"},
          {"err", JSON::Object::Entries {
            {"id", std::to_string(peerId)},
            {"message", String(uv_strerror(err))}
          }}
        };

        return cb(seq, json, Post{});
      }

      auto json = JSON::Object::Entries {
        {"source", "udp.readStop"},
        {"data", JSON::Object::Entries {
          {"id", std::to_string(peerId)}
        }}
      };

      cb(seq, json, Post {});
    });
  }

  void UDP::close (
    const String seq,
    uint64_t peerId,
    Context::Callback cb
  ) {
    this->runtime->dispatchEventLoop([=, this]() {
      if (!this->runtime->hasPeer(peerId)) {
        auto json = ERR_SOCKET_DGRAM_NOT_RUNNING("udp.close", peerId);
        return cb(seq, json, Post{});
      }

      auto peer = this->runtime->getPeer(peerId);

      if (!peer->isUDP()) {
        auto json = ERR_SOCKET_DGRAM_NOT_RUNNING("udp.close", peerId);
        return cb(seq, json, Post{});
      }

      if (peer->isClosed()) {
        auto json = ERR_SOCKET_DGRAM_CLOSED("udp.close", peerId);
        return cb(seq, json, Post{});
      }

      if (peer->isClosing()) {
        auto json = ERR_SOCKET_DGRAM_CLOSING("udp.close", peerId);
        return cb(seq, json, Post{});
      }

      peer->close([=, this]() {
        auto json = JSON::Object::Entries {
          {"source", "udp.close"},
          {"data", JSON::Object::Entries {
            {"id", std::to_string(peerId)}
          }}
        };

        cb(seq, json, Post{});
      });
    });
  }
}