#ifndef SSC_RUNTIME_RUNTIME_HH
#define SSC_RUNTIME_RUNTIME_HH

#include <socket/socket.hh>
#include <uv.h>

namespace ssc::runtime {
  // forward
  class BluetoothInternals;
  class Descriptor;
  class DescriptorManager;
  class FS;
  class Peer;
  class PeerManager;
  class Runtime;
  class Timers;
  namespace ipc {
    class Router;
  }

  constexpr int RECV_BUFFER = 1;
  constexpr int SEND_BUFFER = 0;

  /**
    * A key-value mapping for a single header entry.
    */
  class Header {
    public:
      /**
        * A container for a header value supporting varying types in the
        * constructor. All values are converted to a string.
        */
      class Value {
        public:
          String string;
          Value () = default;
          Value (String value) { this->string = value; }
          Value (const char* value) { this->string = value; }
          Value (const Value& value) { this->string = value.string; }
          Value (bool value) { this->string = value ? "true" : "false"; }
          Value (int value) { this->string = std::to_string(value); }
          Value (float value) { this->string = std::to_string(value); }
          Value (int64_t value) { this->string = std::to_string(value); }
          Value (uint64_t value) { this->string = std::to_string(value); }
          Value (double_t value) { this->string = std::to_string(value); }
          #if defined(__APPLE__)
            Value (ssize_t value) { this->string = std::to_string(value); }
          #endif
          String str () const { return this->string; }
      };

      String key;
      Value value;
      Header () = default;
      Header (const Header& header) {
        this->key = header.key;
        this->value = header.value;
      }

      Header (const String& key, const Value& value) {
        this->key = key;
        this->value = value;
      }
  };

  /**
   * A container for new line delimited headers.
   */
  class Headers {
    public:
      using Entries = Vector<Header>;
      Entries entries;
      Headers () = default;
      Headers (const Headers& headers);
      Headers (const Vector<std::map<String, Header::Value>>& entries);
      Headers (const Entries& entries);
      Headers (const Map& map);
      Headers (const String& source);
      Headers (const char* source);
      void set (const String& key, const String& value);
      bool has (const String& key) const;
      String get (const String& key) const;
      String operator [] (const String& key) const;
      String &operator [] (const String& key);
      size_t size () const;
      String str () const;
      JSON::Object json () const;
  };

  struct Data {
    uint64_t id = 0;
    uint64_t ttl = 0;
    char *body = nullptr;
    size_t length = 0;
    Headers headers = "";
    bool bodyNeedsFree = false;
  };

	class DataManager {
    public:
      using Entries = std::map<uint64_t, Data>;
      SharedPointer<Entries> entries;
      Mutex mutex;

      DataManager (const DataManager& dataManager) = delete;
      DataManager ();

      Data get (uint64_t id);
      void put (uint64_t id, Data& data);
      bool has (uint64_t id);
      void remove (uint64_t id);
      void expire ();
      void clear ();
      String create (const String& seq, const JSON::Any& json, Data& data);
      String create (const String& seq, const String& params, Data& data);
  };

  using NetworkStatusChangeCallback = Function<void(
    const String& statusName,
    const String& statusMessage
  )>;

  class NetworkStatusObserver {
    void *internal = nullptr;
    public:
      NetworkStatusChangeCallback onNetworkStatusChangeCallback = nullptr;
      NetworkStatusObserver ();
      ~NetworkStatusObserver ();
      void onNetworkStatusChange (
        const String& statusName,
        const String& statusMessage
      ) const;
  };

  class Bluetooth {
    public:
      BluetoothInternals* internals = nullptr;
      ipc::Router* router = nullptr;

      Bluetooth (ipc::Router*);
      ~Bluetooth ();
      bool send (const String& seq, JSON::Any json, Data data);
      bool send (const String& seq, JSON::Any json);
      bool emit (const String& seq, JSON::Any json);
      void startScanning ();
      void publishCharacteristic (
        const String& seq,
        char* bytes,
        size_t size,
        const String& serviceId,
        const String& characteristicId
      );
      void subscribeCharacteristic (
        const String& seq,
        const String& serviceId,
        const String& characteristicId
      );
      void startService (
        const String& seq,
        const String& serviceId
      );
  };

  class Script {
    public:
      String name;
      String source;
      Script () = default;
      Script (const Script& script);
      Script (const String& source);
      Script (const String& name, const String& source);

      String str () const;
      JSON::String json () const;
  };

  String createJavaScriptSource (const String& name, const String& source);

  String getEmitToRenderProcessJavaScript (
    const String& event,
    const String& value,
    const String& target,
    const JSON::Object& options
  );

  String getEmitToRenderProcessJavaScript (
    const String& event,
    const String& value
  );

  String getResolveMenuSelectionJavaScript (
    const String& seq,
    const String& title,
    const String& parent
  );

  String getResolveToRenderProcessJavaScript (
    const String& seq,
    const String& state,
    const String& value
  );

  class Loop {
    public:
      using DispatchCallback = std::function<void()>;
      static constexpr int POLL_TIMEOUT = 32; // in milliseconds

      // uv state
      uv_loop_t loop;
      uv_async_t async;
      // instance state
      AtomicBool initialized = false;
      AtomicBool running = false;
      // async state
      Queue<DispatchCallback> queue;
      Thread* thread = nullptr;
      Mutex mutex;

      // async semaphores
      struct {
        BinarySemaphore poll{0};
        BinarySemaphore signal{0};
      } semaphores;

      #if defined(__APPLE__)
        dispatch_queue_t dispatchQueue;
      #endif

      Loop (Loop&) = delete;
      Loop ();
      ~Loop ();

      uv_loop_t* get ();

      bool isInitialized () const;
      bool isAlive ();
      bool isRunning ();

      int timeout ();
      int run (uv_run_mode mode);
      int run ();

      Loop& sleep (int64_t ms);
      Loop& sleep ();
      Loop& wait ();
      Loop& init ();
      Loop& stop ();
      Loop& start ();
      Loop& signal ();
      Loop& dispatch (DispatchCallback callback);
  };

  typedef enum {
    PEER_TYPE_NONE = 0,
    PeerTypeTCP = 1 << 1,
    PEER_TYPE_UDP = 1 << 2,
    PEER_TYPE_MAX = 0xF
  } PeerType;

  typedef enum {
    PEER_FLAG_NONE = 0,
    PEER_FLAG_EPHEMERAL = 1 << 1
  } PeerFlag;

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
    PeerStateCP_BOUND = 1 << 20,
    PeerStateCP_CONNECTED = 1 << 21,
    PeerStateCP_PAUSED = 1 << 13,
    PEER_STATE_MAX = 1 << 0xF
  } PeerState;

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

  /**
   * A generic structure for a bound or connected peer.
   */
  class Peer {
    public:
      struct RequestContext {
        using Callback = Function<void(int, Post)>;
        Callback cb;
        Peer *peer = nullptr;
        RequestContext (Callback cb) { this->cb = cb; }
      };

      using UDPReceiveCallback = Function<void(
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
      Vector<Function<void()>> onclose;

      // instance state
      uint64_t id = 0;
      Mutex mutex;

      struct {
        struct {
          bool reuseAddr = false;
          bool ipv6Only = false; // @TODO
        } udp;
      } options;

      // peer state
      LocalPeerInfo local;
      RemotePeerInfo remote;
      PeerType type = PEER_TYPE_NONE;
      PeerFlag flags = PEER_FLAG_NONE;
      PeerState state = PEER_STATE_NONE;

      PeerManager* peerManager = nullptr;

      /**
      * Private `Peer` class constructor
      */
      Peer (
        PeerManager* peerManager,
        PeerType peerType,
        uint64_t peerId,
        bool isEphemeral
      );
      ~Peer ();

      int init ();
      int initRemotePeerInfo ();
      int initLocalPeerInfo ();
      void addState (PeerState value);
      void removeState (PeerState value);
      bool hasState (PeerState value);
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
      int pause ();
      int resume ();
      void close ();
      void close (Function<void()> onclose);
  };

  class PeerManager {
    public:
      using Peers = std::map<uint64_t, Peer*>;
      Loop& loop;
      Mutex mutex;
      Peers peers;
      PeerManager () = delete;
      PeerManager (Runtime* runtime);
      ~PeerManager () {
        for (auto tuple : this->peers) {
          if (tuple.second != nullptr) {
            this->removePeer(tuple.second->id, true);
          }
        }
      }
      void resumeAllPeers ();
      void pauseAllPeers ();
      bool hasPeer (uint64_t peerId);
      void removePeer (uint64_t peerId);
      void removePeer (uint64_t peerId, bool autoClose);
      Peer* getPeer (uint64_t peerId);
      Peer* createPeer (PeerType peerType, uint64_t peerId);
      Peer* createPeer (PeerType peerType, uint64_t peerId, bool isEphemeral);
  };

  class DNS {
    public:
      using Callback = std::function<void(String, JSON::Any, Data)>;

      struct LookupOptions {
        String hostname;
        int family = 4;
        bool all = false;
        // TODO: support these options
        // - hints
        // -verbatim
      };

      struct RequestContext {
        String seq;
        Callback callback;
        LookupOptions options;
        RequestContext (const String& seq, Callback callback) {
          this->seq = seq;
          this->callback = callback;
        }
      };

      Loop& loop;
      DNS (Runtime* runtime);

      void lookup (const String seq, LookupOptions options, Callback callback);
      void lookup (LookupOptions options, Callback callback);
  };

  struct Descriptor {
    uint64_t id;
    Mutex mutex;
    AtomicBool retained = false;
    AtomicBool stale = false;
    uv_dir_t *dir = nullptr;
    uv_file fd = 0;
    FS* fs = nullptr;

    DescriptorManager* descriptorManager = nullptr;

    Descriptor (uint64_t id, DescriptorManager* descriptorManager) {
      this->id = id;
      this->descriptorManager = descriptorManager;
    }

    bool isDirectory () {
      return this->dir != nullptr;
    }

    bool isFile () {
      Lock lock(this->mutex);
      return this->fd > 0 && this->dir == nullptr;
    }

    bool isRetained () {
      Lock lock(this->mutex);
      return this->retained;
    }

    bool isStale () {
      Lock lock(this->mutex);
      return this->stale;
    }
  };

  struct FSRequestContext {
    using Callback = std::function<void(String, JSON::Any, Data)>;

    uint64_t id;
    String seq;
    Descriptor *desc = nullptr;
    Callback callback = nullptr;
    uv_fs_t req;
    uv_buf_t iov[16];
    // 256 which corresponds to DirectoryHandle.MAX_BUFFER_SIZE
    uv_dirent_t dirents[256];
    int offset = 0;
    int result = 0;
    FS* fs = nullptr;

    FSRequestContext () = default;
    FSRequestContext (Descriptor *desc);
    FSRequestContext (String seq, Callback callback);
    FSRequestContext (Descriptor *desc, String seq, Callback callback);
    ~FSRequestContext ();

    void setBuffer (int index, size_t len, char *base);
    void freeBuffer (int index);
    char* getBuffer (int index);
    size_t getBufferSize (int index);
  };

  class DescriptorManager {
    public:
      using Callback = FSRequestContext::Callback;
      using Descriptors = std::map<uint64_t, Descriptor*>;

      Descriptors descriptors;
      Timers& timers;
      Mutex mutex;
      Loop& loop;
      Runtime* runtime = nullptr;
      FS* fs = nullptr;

      DescriptorManager () = delete;
      DescriptorManager (Runtime* runtime);

      void init ();

      Descriptor* create (uint64_t id);
      Descriptor* get (uint64_t id);
      void remove (uint64_t id);
      bool has (uint64_t id);
      size_t count ();
  };

  class FS {
    public:
      using RequestContext = FSRequestContext;
      using Callback = FSRequestContext::Callback;

      DescriptorManager& descriptorManager;
      Timers& timers;
      Loop& loop;

      FS (Runtime* runtime);

      void constants (const String seq, Callback callback);
      void access (
        const String seq,
        const String path,
        int mode,
        Callback callback
      );
      void chmod (
        const String seq,
        const String path,
        int mode,
        Callback callback
      );
      void close (const String seq, uint64_t id, Callback callback);
      void copyFile (
        const String seq,
        const String src,
        const String dst,
        int mode,
        Callback callback
      );
      void closedir (const String seq, uint64_t id, Callback callback);
      void closeOpenDescriptor (
        const String seq,
        uint64_t id,
        Callback callback
      );
      void closeOpenDescriptors (const String seq, Callback callback);
      void closeOpenDescriptors (
        const String seq,
        bool preserveRetained,
        Callback callback
      );
      void fstat (const String seq, uint64_t id, Callback callback);
      void getOpenDescriptors (const String seq, Callback callback);
      void lstat (const String seq, const String path, Callback callback);
      void mkdir (
        const String seq,
        const String path,
        int mode,
        Callback callback
      );
      void open (
        const String seq,
        uint64_t id,
        const String path,
        int flags,
        int mode,
        Callback callback
      );
      void opendir (
        const String seq,
        uint64_t id,
        const String path,
        Callback callback
      );
      void read (
        const String seq,
        uint64_t id,
        size_t len,
        size_t offset,
        Callback callback
      );
      void readdir (
        const String seq,
        uint64_t id,
        size_t entries,
        Callback callback
      );
      void retainOpenDescriptor (
        const String seq,
        uint64_t id,
        Callback callback
      );
      void rename (
        const String seq,
        const String src,
        const String dst,
        Callback callback
      );
      void rmdir (
        const String seq,
        const String path,
        Callback callback
      );
      void stat (
        const String seq,
        const String path,
        Callback callback
      );
      void unlink (
        const String seq,
        const String path,
        Callback callback
      );
      void write (
        const String seq,
        uint64_t id,
        char *bytes,
        size_t size,
        size_t offset,
        Callback callback
      );
  };

  class OS {
    public:
      using Callback = std::function<void(String, JSON::Any, Data)>;

      PeerManager& peerManager;
      Loop& loop;

      OS (const OS&) = delete;
      OS (Runtime* runtime);

      void bufferSize (
        const String seq,
        uint64_t peerId,
        size_t size,
        int buffer,
        Callback callback
      );

      void networkInterfaces (const String seq, Callback callback) const;
  };

  class Platform {
    public:
      using Callback = Function<void(String, JSON::Any, Data)>;
      struct RequestContext {
        String seq;
        Callback callback;
        RequestContext (const String& seq, Callback callback) {
          this->seq = seq;
          this->callback = callback;
        }
      };

      Runtime* runtime = nullptr;
      Loop& loop;

      Platform (const Platform&) = delete;
      Platform (Runtime* runtime);

      void event (
        const String seq,
        const String event,
        const String data,
        Callback callback
      );

      void notify (
        const String seq,
        const String title,
        const String body,
        Callback callback
      ) const;

      void openExternal (
        const String seq,
        const String value,
        Callback callback
      ) const;

      void cwd (const String seq, Callback callback) const;
  };

  struct Timer {
    uv_timer_t handle;
    bool repeated = false;
    bool started = false;
    uint64_t timeout = 0;
    uint64_t interval = 0;
    uv_timer_cb invoke;
    Timers* timers;
  };

  class Timers {
    Vector<Timer> timers;
    AtomicBool started = false;
    Loop& loop;

    public:
      Timers () = delete;

      Timers (Loop& loop)
        : loop(loop)
      {}

      void add (Timer& timer, void* data);
      void start ();
      void stop ();
  };

  class UDP {
    public:
      using Callback = std::function<void(String, JSON::Any, Data)>;

      PeerManager& peerManager;
      Loop& loop;

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

      UDP (Runtime* runtime);

      void bind (
        const String seq,
        uint64_t id,
        BindOptions options,
        Callback callback
      );
      void close (const String seq, uint64_t id, Callback callback);
      void connect (
        const String seq,
        uint64_t id,
        ConnectOptions options,
        Callback callback
      );
      void disconnect (const String seq, uint64_t id, Callback callback);
      void getPeerName (const String seq, uint64_t id, Callback callback);
      void getSockName (const String seq, uint64_t id, Callback callback);
      void getState (const String seq, uint64_t id,  Callback callback);
      void readStart (const String seq, uint64_t id, Callback callback);
      void readStop (const String seq, uint64_t id, Callback callback);
      void send (
        const String seq,
        uint64_t id,
        SendOptions options,
        Callback callback
      );
  };

  class Runtime {
    public:
      DescriptorManager descriptorManager;
      DataManager dataManager;
      PeerManager peerManager;
      Timers timers;

      struct {
        Loop main;
        Loop background;
      } loops;

      DNS dns;
      FS fs;
      OS os;
      Platform platform;
      UDP udp;

      Runtime (const Runtime&) = delete;

      Runtime ()
        : platform(this),
          timers(this->loops.background),
          descriptorManager(this),
          peerManager(this),
          dns(this),
          fs(this),
          os(this),
          udp(this)
      {}

      void start ();
      void stop ();
      void dispatch (Loop::DispatchCallback cb);
      void wait ();
  };
}
#endif
