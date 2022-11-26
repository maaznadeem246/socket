#include <socket/runtime.hh>
#include <socket/utils.hh>

using namespace ssc::utils;

namespace ssc::runtime {
  JSON::Object::Entries ERR_SOCKET_ALREADY_BOUND (
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

  JSON::Object::Entries ERR_SOCKET_DGRAM_IS_CONNECTED (
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

  JSON::Object::Entries ERR_SOCKET_DGRAM_NOT_CONNECTED (
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

  JSON::Object::Entries ERR_SOCKET_DGRAM_CLOSED (
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

  JSON::Object::Entries ERR_SOCKET_DGRAM_CLOSING (
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

  JSON::Object::Entries ERR_SOCKET_DGRAM_NOT_RUNNING (
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

  UDP::UDP (Runtime* runtime)
    : loop(runtime->loops.main),
      peerManager(runtime->peerManager)
  {}

  void UDP::bind (
    const String seq,
    uint64_t peerId,
    UDP::BindOptions options,
    Callback callback
  ) {
    this->loop.dispatch([=, this]() {
      if (this->peerManager.hasPeer(peerId)) {
        if (this->peerManager.getPeer(peerId)->isBound()) {
          auto json = ERR_SOCKET_ALREADY_BOUND("udp.bind", peerId);
          return callback(seq, json, Data{});
        }
      }

      auto peer = this->peerManager.createPeer(PEER_TYPE_UDP, peerId);
      auto err = peer->bind(options.address, options.port, options.reuseAddr);

      if (err < 0) {
        auto json = JSON::Object::Entries {
          {"source", "udp.bind"},
          {"err", JSON::Object::Entries {
            {"id", std::to_string(peerId)},
            {"message", String(uv_strerror(err))}
          }}
        };

        return callback(seq, json, Data{});
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

        return callback(seq, json, Data{});
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

      callback(seq, json, Data{});
    });
  }

  void UDP::connect (
    const String seq,
    uint64_t peerId,
    UDP::ConnectOptions options,
    Callback callback
  ) {
    this->loop.dispatch([=, this]() {
      auto peer = this->peerManager.createPeer(PEER_TYPE_UDP, peerId);

      if (peer->isConnected()) {
        auto json = ERR_SOCKET_DGRAM_IS_CONNECTED("udp.connect", peerId);
        return callback(seq, json, Data{});
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

        return callback(seq, json, Data{});
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

        return callback(seq, json, Data{});
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

      callback(seq, json, Data{});
    });
  }

  void UDP::disconnect (
    const String seq,
    uint64_t peerId,
    Callback callback
  ) {
    this->loop.dispatch([=, this]() {
      if (!this->peerManager.hasPeer(peerId)) {
        auto json = ERR_SOCKET_DGRAM_NOT_CONNECTED("udp.disconnect", peerId);
        return callback(seq, json, Data{});
      }

      auto peer = this->peerManager.getPeer(peerId);
      auto err = peer->disconnect();

      if (err < 0) {
        auto json = JSON::Object::Entries {
          {"source", "udp.disconnect"},
          {"err", JSON::Object::Entries {
            {"id", std::to_string(peerId)},
            {"message", String(uv_strerror(err))}
          }}
        };

        return callback(seq, json, Data{});
      }

      auto json = JSON::Object::Entries {
        {"source", "udp.disconnect"},
        {"data", JSON::Object::Entries {
          {"id", std::to_string(peerId)}
        }}
      };

      callback(seq, json, Data{});
    });
  }

  void UDP::getPeerName (String seq, uint64_t peerId, Callback callback) {
    if (!this->peerManager.hasPeer(peerId)) {
      auto json = ERR_SOCKET_DGRAM_NOT_CONNECTED("udp.getPeerName", peerId);
      return callback(seq, json, Data{});
    }

    auto peer = this->peerManager.getPeer(peerId);
    auto info = peer->getRemotePeerInfo();

    if (info->err < 0) {
      auto json = JSON::Object::Entries {
        {"source", "udp.getPeerName"},
        {"err", JSON::Object::Entries {
          {"id", std::to_string(peerId)},
          {"message", String(uv_strerror(info->err))}
        }}
      };

      return callback(seq, json, Data{});
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

    callback(seq, json, Data{});
  }

  void UDP::getSockName (String seq, uint64_t peerId, Callback callback) {
    if (!this->peerManager.hasPeer(peerId)) {
      auto json = ERR_SOCKET_DGRAM_NOT_RUNNING("udp.getSockName", peerId);
      return callback(seq, json, Data{});
    }

    auto peer = this->peerManager.getPeer(peerId);
    auto info = peer->getLocalPeerInfo();

    if (info->err < 0) {
      auto json = JSON::Object::Entries {
        {"source", "udp.getSockName"},
        {"err", JSON::Object::Entries {
          {"id", std::to_string(peerId)},
          {"message", String(uv_strerror(info->err))}
        }}
      };

      return callback(seq, json, Data{});
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

    callback(seq, json, Data{});
  }

  void UDP::getState (
    const String seq,
    uint64_t peerId,
    Callback callback
  ) {
    if (!this->peerManager.hasPeer(peerId)) {
      auto json = ERR_SOCKET_DGRAM_NOT_RUNNING("udp.getState", peerId);
      return callback(seq, json, Data{});
    }

    auto peer = this->peerManager.getPeer(peerId);

    if (!peer->isUDP()) {
      auto json = ERR_SOCKET_DGRAM_NOT_RUNNING("udp.getState", peerId);
      return callback(seq, json, Data{});
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

    callback(seq, json, Data{});
  }

  void UDP::send (
    String seq,
    uint64_t peerId,
    UDP::SendOptions options,
    Callback callback
  ) {
    this->loop.dispatch([=, this] {
      auto peer = this->peerManager.createPeer(PEER_TYPE_UDP, peerId, options.ephemeral);
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

          return callback(seq, json, Data{});
        }

        auto json = JSON::Object::Entries {
          {"source", "udp.send"},
          {"data", JSON::Object::Entries {
            {"id", std::to_string(peerId)},
            {"status", status}
          }}
        };

        callback(seq, json, Data{});
      });
    });
  }

  void UDP::readStart (String seq, uint64_t peerId, Callback callback) {
    if (!this->peerManager.hasPeer(peerId)) {
      auto json = ERR_SOCKET_DGRAM_NOT_RUNNING("udp.readStart", peerId);
      return callback(seq, json, Data{});
    }

    auto peer = this->peerManager.getPeer(peerId);

    if (peer->isClosed()) {
      auto json = ERR_SOCKET_DGRAM_CLOSED("udp.readStart", peerId);
      return callback(seq, json, Data{});
    }

    if (peer->isClosing()) {
      auto json = ERR_SOCKET_DGRAM_CLOSING("udp.readStart", peerId);
      return callback(seq, json, Data{});
    }

    if (peer->hasState(PEER_STATE_UDP_RECV_STARTED)) {
      auto json = JSON::Object::Entries {
        {"source", "udp.readStart"},
        {"err", JSON::Object::Entries {
          {"id", std::to_string(peerId)},
          {"message", "Socket is already receiving"}
        }}
      };

      return callback(seq, json, Data{});
    }

    if (peer->isActive()) {
      auto json = JSON::Object::Entries {
        {"source", "udp.readStart"},
        {"data", JSON::Object::Entries {
          {"id", std::to_string(peerId)}
        }}
      };

      return callback(seq, json, Data{});
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

        return callback("-1", json, Data{});
      }

      if (nread > 0) {
        char address[17];
        Data post;
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

        return callback("-1", json, post);
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

      return callback(seq, json, Data{});
    }

    auto json = JSON::Object::Entries {
      {"source", "udp.readStart"},
      {"data", JSON::Object::Entries {
        {"id", std::to_string(peerId)}
      }}
    };

    callback(seq, json, Data {});
  }

  void UDP::readStop (
    const String seq,
    uint64_t peerId,
    Callback callback
  ) {
    this->loop.dispatch([=, this] {
      if (!this->peerManager.hasPeer(peerId)) {
        auto json = ERR_SOCKET_DGRAM_NOT_RUNNING("udp.readStop", peerId);
        return callback(seq, json, Data{});
      }

      auto peer = this->peerManager.getPeer(peerId);

      if (peer->isClosed()) {
        auto json = ERR_SOCKET_DGRAM_CLOSED("udp.readStop", peerId);
        return callback(seq, json, Data{});
      }

      if (peer->isClosing()) {
        auto json = ERR_SOCKET_DGRAM_CLOSING("udp.readStop", peerId);
        return callback(seq, json, Data{});
      }

      if (!peer->hasState(PEER_STATE_UDP_RECV_STARTED)) {
        auto json = JSON::Object::Entries {
          {"source", "udp.readStop"},
          {"err", JSON::Object::Entries {
            {"id", std::to_string(peerId)},
            {"message", "Socket is not receiving"}
          }}
        };

        return callback(seq, json, Data{});
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

        return callback(seq, json, Data{});
      }

      auto json = JSON::Object::Entries {
        {"source", "udp.readStop"},
        {"data", JSON::Object::Entries {
          {"id", std::to_string(peerId)}
        }}
      };

      callback(seq, json, Data {});
    });
  }

  void UDP::close (
    const String seq,
    uint64_t peerId,
    Callback callback
  ) {
    this->loop.dispatch([=, this]() {
      if (!this->peerManager.hasPeer(peerId)) {
        auto json = ERR_SOCKET_DGRAM_NOT_RUNNING("udp.close", peerId);
        return callback(seq, json, Data{});
      }

      auto peer = this->peerManager.getPeer(peerId);

      if (!peer->isUDP()) {
        auto json = ERR_SOCKET_DGRAM_NOT_RUNNING("udp.close", peerId);
        return callback(seq, json, Data{});
      }

      if (peer->isClosed()) {
        auto json = ERR_SOCKET_DGRAM_CLOSED("udp.close", peerId);
        return callback(seq, json, Data{});
      }

      if (peer->isClosing()) {
        auto json = ERR_SOCKET_DGRAM_CLOSING("udp.close", peerId);
        return callback(seq, json, Data{});
      }

      peer->close([=, this]() {
        auto json = JSON::Object::Entries {
          {"source", "udp.close"},
          {"data", JSON::Object::Entries {
            {"id", std::to_string(peerId)}
          }}
        };

        callback(seq, json, Data{});
      });
    });
  }
}
