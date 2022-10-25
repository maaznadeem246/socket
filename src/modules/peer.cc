module;

#include "../platform.hh"

import :interfaces;
import :json;

export module ssc.runtime:peer;

export namespace ssc {
  typedef enum {
    PEER_TYPE_NONE = 0,
    PEER_TYPE_TCP = 1 << 1,
    PEER_TYPE_UDP = 1 << 2,
    PEER_TYPE_MAX = 0xF
  } peer_type_t;

  typedef enum {
    PEER_FLAG_NONE = 0,
    PEER_FLAG_EPHEMERAL = 1 << 1
  } peer_flag_t;

  typedef enum {
    PEER_STATE_NONE = 0,
    // general states
    PEER_STATE_CLOSED = 1 << 1,
    // udp states (10)
    PEER_STATE_UDP_BOUND = 1 << 10,
    PEER_STATE_UDP_CONNECTED = 1 << 11,
    PEER_STATE_UDP_RECV_STARTED = 1 << 12,
    PEER_STATE_UDP_PAUSED = 1 << 13,
    // tcp states (20)
    PEER_STATE_TCP_BOUND = 1 << 20,
    PEER_STATE_TCP_CONNECTED = 1 << 21,
    PEER_STATE_TCP_PAUSED = 1 << 13,
    PEER_STATE_MAX = 1 << 0xF
  } peer_state_t;

  struct LocalPeerInfo {
    struct sockaddr_storage addr;
    String address = "";
    String family = "";
    int port = 0;
    int err = 0;

    int getsockname (uv_udp_t *socket, struct sockaddr *addr);
    int getsockname (uv_tcp_t *socket, struct sockaddr *addr);
    void init (uv_udp_t *socket);
    void init (uv_tcp_t *socket);
    void init (const struct sockaddr_storage *addr);
  };

  struct RemotePeerInfo {
    struct sockaddr_storage addr;
    String address = "";
    String family = "";
    int port = 0;
    int err = 0;

    int getpeername (uv_udp_t *socket, struct sockaddr *addr);
    int getpeername (uv_tcp_t *socket, struct sockaddr *addr);
    void init (uv_udp_t *socket);
    void init (uv_tcp_t *socket);
    void init (const struct sockaddr_storage *addr);
  };

  static inline String addrToIPv4 (struct sockaddr_in* sin) {
    char buf[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &sin->sin_addr, buf, INET_ADDRSTRLEN);
    return String(buf);
  }

  static inline String addrToIPv6 (struct sockaddr_in6* sin) {
    char buf[INET6_ADDRSTRLEN];
    inet_ntop(AF_INET6, &sin->sin6_addr, buf, INET6_ADDRSTRLEN);
    return String(buf);
  }

  static inline void parseAddress (struct sockaddr *name, int* port, char* address) {
    struct sockaddr_in *name_in = (struct sockaddr_in *) name;
    *port = ntohs(name_in->sin_port);
    uv_ip4_name(name_in, address, 17);
  }

  /**
   * A generic structure for a bound or connected peer.
   */
  class Peer {
    public:
      struct RequestContext {
        using Callback = std::function<void(int, Post)>;
        Callback cb;
        Peer *peer = nullptr;
        RequestContext (Callback cb) { this->cb = cb; }
      };

      using UDPReceiveCallback = std::function<void(
        ssize_t,
        const uv_buf_t*,
        const struct sockaddr*
      )>;

      // uv handles
      union {
        uv_udp_t udp;
        uv_tcp_t tcp; // XXX: FIXME
      } handle;

      // sockaddr
      struct sockaddr_in addr;

      // callbacks
      UDPReceiveCallback receiveCallback;
      std::vector<std::function<void()>> onclose;

      // instance state
      uint64_t id = 0;
      std::recursive_mutex mutex;
      Runtime *runtime;

      struct {
        struct {
          bool reuseAddr = false;
          bool ipv6Only = false; // @TODO
        } udp;
      } options;

      // peer state
      LocalPeerInfo local;
      RemotePeerInfo remote;
      peer_type_t type = PEER_TYPE_NONE;
      peer_flag_t flags = PEER_FLAG_NONE;
      peer_state_t state = PEER_STATE_NONE;

      /**
      * Private `Peer` class constructor
      */
      Peer (Runtime *runtime, peer_type_t peerType, uint64_t peerId, bool isEphemeral);
      ~Peer ();

      int init ();
      int initRemotePeerInfo ();
      int initLocalPeerInfo ();
      void addState (peer_state_t value);
      void removeState (peer_state_t value);
      bool hasState (peer_state_t value);
      const RemotePeerInfo* getRemotePeerInfo ();
      const LocalPeerInfo* getLocalPeerInfo ();
      bool isUDP ();
      bool isTCP ();
      bool isEphemeral ();
      bool isBound ();
      bool isActive ();
      bool isClosing ();
      bool isClosed ();
      bool isConnected ();
      bool isPaused ();
      int bind ();
      int bind (String address, int port);
      int bind (String address, int port, bool reuseAddr);
      int rebind ();
      int connect (String address, int port);
      int disconnect ();
      void send (
        char *buf,
        size_t size,
        int port,
        const String address,
        Peer::RequestContext::Callback cb
      );
      int recvstart ();
      int recvstart (UDPReceiveCallback onrecv);
      int recvstop ();
      int resume ();
      int pause ();
      void close ();
      void close (std::function<void()> onclose);
  };

  void Runtime::resumeAllPeers () {
    dispatchEventLoop([=, this]() {
      Lock lock(this->peersMutex);
      for (auto const &tuple : this->peers) {
        auto peer = tuple.second;
        if (peer != nullptr && (peer->isBound() || peer->isConnected())) {
          peer->resume();
        }
      }
    });
  }

  void Runtime::pauseAllPeers () {
    dispatchEventLoop([=, this]() {
      Lock lock(this->peersMutex);
      for (auto const &tuple : this->peers) {
        auto peer = tuple.second;
        if (peer != nullptr && (peer->isBound() || peer->isConnected())) {
          peer->pause();
        }
      }
    });
  }

  bool Runtime::hasPeer (uint64_t peerId) {
    Lock lock(this->peersMutex);
    return this->peers.find(peerId) != this->peers.end();
  }

  void Runtime::removePeer (uint64_t peerId) {
    return this->removePeer(peerId, false);
  }

  void Runtime::removePeer (uint64_t peerId, bool autoClose) {
    if (this->hasPeer(peerId)) {
      if (autoClose) {
        auto peer = this->getPeer(peerId);
        if (peer != nullptr) {
          peer->close();
        }
      }

      Lock lock(this->peersMutex);
      this->peers.erase(peerId);
    }
  }

  Peer* Runtime::getPeer (uint64_t peerId) {
    if (!this->hasPeer(peerId)) return nullptr;
    Lock lock(this->peersMutex);
    return this->peers.at(peerId);
  }

  Peer* Runtime::createPeer (peer_type_t peerType, uint64_t peerId) {
    return this->createPeer(peerType, peerId, false);
  }

  Peer* Runtime::createPeer (
    peer_type_t peerType,
    uint64_t peerId,
    bool isEphemeral
  ) {
    if (this->hasPeer(peerId)) {
      auto peer = this->getPeer(peerId);
      if (peer != nullptr) {
        if (isEphemeral) {
          Lock lock(peer->mutex);
          peer->flags = (peer_flag_t) (peer->flags | PEER_FLAG_EPHEMERAL);
        }
      }

      return peer;
    }

    auto peer = new Peer(this, peerType, peerId, isEphemeral);
    Lock lock(this->peersMutex);
    this->peers[peer->id] = peer;
    return peer;
  }

  int LocalPeerInfo::getsockname (uv_udp_t *socket, struct sockaddr *addr) {
    int namelen = sizeof(struct sockaddr_storage);
    return uv_udp_getsockname(socket, addr, &namelen);
  }

  int LocalPeerInfo::getsockname (uv_tcp_t *socket, struct sockaddr *addr) {
    int namelen = sizeof(struct sockaddr_storage);
    return uv_tcp_getsockname(socket, addr, &namelen);
  }

  void LocalPeerInfo::init (uv_udp_t *socket) {
    this->address = "";
    this->family = "";
    this->port = 0;

    if ((this->err = getsockname(socket, (struct sockaddr *) &this->addr))) {
      return;
    }

    this->init(&this->addr);
  }

  void LocalPeerInfo::init (uv_tcp_t *socket) {
    this->address = "";
    this->family = "";
    this->port = 0;

    if ((this->err = getsockname(socket, (struct sockaddr *) &this->addr))) {
      return;
    }

    this->init(&this->addr);
  }

  void LocalPeerInfo::init (const struct sockaddr_storage *addr) {
    if (addr->ss_family == AF_INET) {
      this->family = "IPv4";
      this->address = addrToIPv4((struct sockaddr_in*) addr);
      this->port = (int) htons(((struct sockaddr_in*) addr)->sin_port);
    } else if (addr->ss_family == AF_INET6) {
      this->family = "IPv6";
      this->address = addrToIPv6((struct sockaddr_in6*) addr);
      this->port = (int) htons(((struct sockaddr_in6*) addr)->sin6_port);
    }
  }

  int RemotePeerInfo::getpeername (uv_udp_t *socket, struct sockaddr *addr) {
    int namelen = sizeof(struct sockaddr_storage);
    return uv_udp_getpeername(socket, addr, &namelen);
  }

  int RemotePeerInfo::getpeername (uv_tcp_t *socket, struct sockaddr *addr) {
    int namelen = sizeof(struct sockaddr_storage);
    return uv_tcp_getpeername(socket, addr, &namelen);
  }

  void RemotePeerInfo::init (uv_udp_t *socket) {
    this->address = "";
    this->family = "";
    this->port = 0;

    if ((this->err = getpeername(socket, (struct sockaddr *) &this->addr))) {
      return;
    }

    this->init(&this->addr);
  }

  void RemotePeerInfo::init (uv_tcp_t *socket) {
    this->address = "";
    this->family = "";
    this->port = 0;

    if ((this->err = getpeername(socket, (struct sockaddr *) &this->addr))) {
      return;
    }

    this->init(&this->addr);
  }

  void RemotePeerInfo::init (const struct sockaddr_storage *addr) {
    if (addr->ss_family == AF_INET) {
      this->family = "IPv4";
      this->address = addrToIPv4((struct sockaddr_in*) addr);
      this->port = (int) htons(((struct sockaddr_in*) addr)->sin_port);
    } else if (addr->ss_family == AF_INET6) {
      this->family = "IPv6";
      this->address = addrToIPv6((struct sockaddr_in6*) addr);
      this->port = (int) htons(((struct sockaddr_in6*) addr)->sin6_port);
    }
  }

  Peer::Peer (
    Runtime *runtime,
    peer_type_t peerType,
    uint64_t peerId,
    bool isEphemeral
  ) {
    this->id = peerId;
    this->type = peerType;
    this->runtime = runtime;

    if (isEphemeral) {
      this->flags = (peer_flag_t) (this->flags | PEER_FLAG_EPHEMERAL);
    }

    this->init();
  }

  Peer::~Peer () {
    this->runtime->removePeer(this->id, true); // auto close
  }

  int Peer::init () {
    Lock lock(this->mutex);
    auto loop = this->runtime->getEventLoop();
    int err = 0;

    memset(&this->handle, 0, sizeof(this->handle));

    if (this->type == PEER_TYPE_UDP) {
      if ((err = uv_udp_init(loop, (uv_udp_t *) &this->handle))) {
        return err;
      }
      this->handle.udp.data = (void *) this;
    } else if (this->type == PEER_TYPE_TCP) {
      if ((err = uv_tcp_init(loop, (uv_tcp_t *) &this->handle))) {
        return err;
      }
      this->handle.tcp.data = (void *) this;
    }

    return err;
  }

  int Peer::initRemotePeerInfo () {
    Lock lock(this->mutex);
    if (this->type == PEER_TYPE_UDP) {
      this->remote.init((uv_udp_t *) &this->handle);
    } else if (this->type == PEER_TYPE_TCP) {
      this->remote.init((uv_tcp_t *) &this->handle);
    }
    return this->remote.err;
  }

  int Peer::initLocalPeerInfo () {
    Lock lock(this->mutex);
    if (this->type == PEER_TYPE_UDP) {
      this->local.init((uv_udp_t *) &this->handle);
    } else if (this->type == PEER_TYPE_TCP) {
      this->local.init((uv_tcp_t *) &this->handle);
    }
    return this->local.err;
  }

  void Peer::addState (peer_state_t value) {
    Lock lock(this->mutex);
    this->state = (peer_state_t) (this->state | value);
  }

  void Peer::removeState (peer_state_t value) {
    Lock lock(this->mutex);
    this->state = (peer_state_t) (this->state & ~value);
  }

  bool Peer::hasState (peer_state_t value) {
    Lock lock(this->mutex);
    return (value & this->state) == value;
  }

  const RemotePeerInfo* Peer::getRemotePeerInfo () {
    Lock lock(this->mutex);
    return &this->remote;
  }

  const LocalPeerInfo* Peer::getLocalPeerInfo () {
    Lock lock(this->mutex);
    return &this->local;
  }

  bool Peer::isUDP () {
    Lock lock(this->mutex);
    return this->type == PEER_TYPE_UDP;
  }

  bool Peer::isTCP () {
    Lock lock(this->mutex);
    return this->type == PEER_TYPE_TCP;
  }

  bool Peer::isEphemeral () {
    Lock lock(this->mutex);
    return (PEER_FLAG_EPHEMERAL & this->flags) == PEER_FLAG_EPHEMERAL;
  }

  bool Peer::isBound () {
    return (
      (this->isUDP() && this->hasState(PEER_STATE_UDP_BOUND)) ||
      (this->isTCP() && this->hasState(PEER_STATE_TCP_BOUND))
    );
  }

  bool Peer::isActive () {
    Lock lock(this->mutex);
    return uv_is_active((const uv_handle_t *) &this->handle);
  }

  bool Peer::isClosing () {
    Lock lock(this->mutex);
    return uv_is_closing((const uv_handle_t *) &this->handle);
  }

  bool Peer::isClosed () {
    return this->hasState(PEER_STATE_CLOSED);
  }

  bool Peer::isConnected () {
    return (
      (this->isUDP() && this->hasState(PEER_STATE_UDP_CONNECTED)) ||
      (this->isTCP() && this->hasState(PEER_STATE_TCP_CONNECTED))
    );
  }

  bool Peer::isPaused () {
    return (
      (this->isUDP() && this->hasState(PEER_STATE_UDP_PAUSED)) ||
      (this->isTCP() && this->hasState(PEER_STATE_TCP_PAUSED))
    );
  }

  int Peer::bind () {
    auto info = this->getLocalPeerInfo();

    if (info->err) {
      return info->err;
    }

    return this->bind(info->address, info->port, this->options.udp.reuseAddr);
  }

  int Peer::bind (const String address, int port) {
    return this->bind(address, port, false);
  }

  int Peer::bind (const String address, int port, bool reuseAddr) {
    Lock lock(this->mutex);
    auto sockaddr = (struct sockaddr*) &this->addr;
    int flags = 0;
    int err = 0;

    this->options.udp.reuseAddr = reuseAddr;

    if (reuseAddr) {
      flags |= UV_UDP_REUSEADDR;
    }

    if (this->isUDP()) {
      if ((err = uv_ip4_addr((char *) address.c_str(), port, &this->addr))) {
        return err;
      }

      // @TODO(jwerle): support flags in `bind()`
      if ((err = uv_udp_bind((uv_udp_t *) &this->handle, sockaddr, flags))) {
        return err;
      }

      this->addState(PEER_STATE_UDP_BOUND);
    }

    if (this->isTCP()) {
      // @TODO: `bind()` + `listen()?`
    }

    return this->initLocalPeerInfo();
  }

  int Peer::rebind () {
    int err = 0;

    if (this->isUDP()) {
      if ((err = this->recvstop())) {
        return err;
      }
    }

    Lock lock(this->mutex);
    memset((void *) &this->addr, 0, sizeof(struct sockaddr_in));

    if ((err = this->bind())) {
      return err;
    }

    if (this->isUDP()) {
      if ((err = this->recvstart())) {
        return err;
      }
    }

    return err;
  }

  int Peer::connect (const String address, int port) {
    Lock lock(this->mutex);
    auto sockaddr = (struct sockaddr*) &this->addr;
    int err = 0;

    if ((err = uv_ip4_addr((char *) address.c_str(), port, &this->addr))) {
      return err;
    }

    if (this->isUDP()) {
      if ((err = uv_udp_connect((uv_udp_t *) &this->handle, sockaddr))) {
        return err;
      }

      this->addState(PEER_STATE_UDP_CONNECTED);
    }

    return this->initRemotePeerInfo();
  }

  int Peer::disconnect () {
    int err = 0;

    if (this->isUDP()) {
      if (this->isConnected()) {
        // calling `uv_udp_connect()` with `nullptr` for `addr`
        // should disconnect the connected socket
        Lock lock(this->mutex);
        if ((err = uv_udp_connect((uv_udp_t *) &this->handle, nullptr))) {
          return err;
        }

        this->removeState(PEER_STATE_UDP_CONNECTED);
      }
    }

    return err;
  }

  void Peer::send (
    char *buf,
    size_t size,
    int port,
    const String address,
    Peer::RequestContext::Callback cb
  ) {
    Lock lock(this->mutex);
    int err = 0;

    struct sockaddr *sockaddr = nullptr;

    if (!this->isConnected()) {
      sockaddr = (struct sockaddr *) &this->addr;
      err = uv_ip4_addr((char *) address.c_str(), port, &this->addr);

      if (err) {
        return cb(err, Post{});
      }
    }

    auto buffer = uv_buf_init(buf, (int) size);
    auto ctx = new Peer::RequestContext(cb);
    auto req = new uv_udp_send_t;

    req->data = (void *) ctx;
    ctx->peer = this;

    err = uv_udp_send(req, (uv_udp_t *) &this->handle, &buffer, 1, sockaddr, [](uv_udp_send_t *req, int status) {
      auto ctx = reinterpret_cast<Peer::RequestContext*>(req->data);
      auto peer = ctx->peer;

      ctx->cb(status, Post{});

      if (peer->isEphemeral()) {
        peer->close();
      }

      delete ctx;
      delete req;
    });

    if (err < 0) {
      ctx->cb(err, Post{});

      if (this->isEphemeral()) {
        this->close();
      }

      delete ctx;
      delete req;
    }
  }

  int Peer::recvstart () {
    if (this->receiveCallback != nullptr) {
      return this->recvstart(this->receiveCallback);
    }

    return UV_EINVAL;
  }

  int Peer::recvstart (Peer::UDPReceiveCallback receiveCallback) {
    Lock lock(this->mutex);

    if (this->hasState(PEER_STATE_UDP_RECV_STARTED)) {
      return UV_EALREADY;
    }

    this->addState(PEER_STATE_UDP_RECV_STARTED);
    this->receiveCallback = receiveCallback;

    auto allocate = [](uv_handle_t *handle, size_t size, uv_buf_t *buf) {
      if (size > 0) {
        buf->base = (char *) new char[size]{0};
        buf->len = size;
        memset(buf->base, 0, buf->len);
      }
    };

    auto receive = [](
      uv_udp_t *handle,
      ssize_t nread,
      const uv_buf_t *buf,
      const struct sockaddr *addr,
      unsigned flags
    ) {
      auto peer = (Peer *) handle->data;

      if (nread <= 0) {
        if (buf && buf->base) {
          free(buf->base);
        }
      }

      if (nread == UV_ENOTCONN) {
        peer->recvstop();
        return;
      }

      peer->receiveCallback(nread, buf, addr);
    };

    return uv_udp_recv_start((uv_udp_t *) &this->handle, allocate, receive);
  }

  int Peer::recvstop () {
    int err = 0;

    if (this->hasState(PEER_STATE_UDP_RECV_STARTED)) {
      this->removeState(PEER_STATE_UDP_RECV_STARTED);
      Lock lock(this->runtime->loopMutex);
      err = uv_udp_recv_stop((uv_udp_t *) &this->handle);
    }

    return err;
  }

  int Peer::resume () {
    int err = 0;

    if (this->isPaused()) {
      if ((err = this->init())) {
        return err;
      }

      if (this->isBound()) {
        if ((err = this->rebind())) {
          return err;
        }
      } else if (this->isConnected()) {
        // @TODO
      }

      this->removeState(PEER_STATE_UDP_PAUSED);
    }

    return err;
  }

  int Peer::pause () {
    int err = 0;

    if ((err = this->recvstop())) {
      return err;
    }

    if (!this->isPaused() && !this->isClosing()) {
      this->addState(PEER_STATE_UDP_PAUSED);
      if (this->isBound()) {
        Lock lock(this->mutex);
        uv_close((uv_handle_t *) &this->handle, nullptr);
      } else if (this->isConnected()) {
        // TODO
      }
    }

    return err;
  }

  void Peer::close () {
    return this->close(nullptr);
  }

  void Peer::close (std::function<void()> onclose) {
    if (this->isClosed()) {
      this->runtime->removePeer(this->id);
      onclose();
      return;
    }

    if (onclose != nullptr) {
      Lock lock(this->mutex);
      this->onclose.push_back(onclose);
    }

    if (this->isClosing()) {
      return;
    }

    if (this->type == PEER_TYPE_UDP) {
      Lock lock(this->mutex);
      // reset state and set to CLOSED
      uv_close((uv_handle_t*) &this->handle, [](uv_handle_t *handle) {
        auto peer = (Peer *) handle->data;
        if (peer != nullptr) {
          peer->removeState((peer_state_t) (
            PEER_STATE_UDP_BOUND |
            PEER_STATE_UDP_CONNECTED |
            PEER_STATE_UDP_RECV_STARTED
          ));

          for (const auto &onclose : peer->onclose) {
            onclose();
          }

          delete peer;
        }
      });
    }
  }
}
