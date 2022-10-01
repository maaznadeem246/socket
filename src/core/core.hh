#ifndef SSC_CORE_CORE_H
#define SSC_CORE_CORE_H

#include "common.hh"
#include "include/uv.h"

#ifndef _WIN32
#include <ifaddrs.h>
#include <sys/types.h>
#include <arpa/inet.h>
#endif

#if defined(__linux__) && !defined(__ANDROID__)
#include <gtk/gtk.h>
#endif

#include <chrono>
#include <semaphore>
#include <thread>
#include <queue>

#if defined(__APPLE__)
#import <Webkit/Webkit.h>
#import <Network/Network.h>
#import <CoreBluetooth/CoreBluetooth.h>
#import <UserNotifications/UserNotifications.h>

#ifndef debug
#define debug(format, ...) NSLog(@format, ##__VA_ARGS__)
#endif
#endif

#ifndef debug
#  define debug(format, ...) fprintf(stderr, format "\n", ##__VA_ARGS__)
#endif

namespace SSC {
#if defined(__APPLE__)
  using Task = id<WKURLSchemeTask>;
#else
  class Task {};
#endif

  using ID = uint64_t;
  using String = std::string;
  using Tasks = std::map<String, Task>;
  using EventLoopDispatchCallback = std::function<void()>;

  constexpr int EVENT_LOOP_POLL_TIMEOUT = 32; // in milliseconds

  // forward
  class Core;
  struct Peer;
  struct Descriptor;

  struct Post {
    uint64_t id = 0;
    uint64_t ttl = 0;
    char* body = nullptr;
    int length = 0;
    String headers = "";
    bool bodyNeedsFree = false;
  };

  using Posts = std::map<uint64_t, Post>;
  using Callback = std::function<void(String, String, Post)>;

  struct Descriptor {
    Core *core;
    uv_file fd = 0;
    uv_dir_t *dir = nullptr;
    String seq;
    Callback cb;
    uint64_t id;
    std::atomic<bool> retained = false;
    std::atomic<bool> stale = false;
    std::recursive_mutex mutex;
    void *data;

    Descriptor (Core *core, uint64_t id);

    bool isDirectory ();
    bool isFile ();
    bool isRetained ();
    bool isStale ();
  };

  struct DescriptorRequestContext {
    uint64_t id;
    String seq = "";
    Descriptor *desc = nullptr;
    uv_fs_t req;
    uv_buf_t iov[16];
    // 256 which corresponds to DirectoryHandle.MAX_BUFFER_SIZE
    uv_dirent_t dirents[256];
    int offset = 0;
    int result = 0;
    Callback cb;

    DescriptorRequestContext () {}
    DescriptorRequestContext (Descriptor *desc)
      : DescriptorRequestContext(desc, "", nullptr) {}
    DescriptorRequestContext (String seq, Callback cb)
      : DescriptorRequestContext(nullptr, seq, cb) {}
    DescriptorRequestContext (Descriptor *desc, String seq, Callback cb) {
      this->id = SSC::rand64();
      this->cb = cb;
      this->seq = seq;
      this->desc = desc;
      this->req.data = (void *) this;
    }

    ~DescriptorRequestContext () {
      uv_fs_req_cleanup(&this->req);
    }

    void setBuffer (int index, int len, char *base);
    void freeBuffer (int index);
    char* getBuffer (int index);
    size_t getBufferSize (int index);
    void end (String seq, String msg, Post post);
    void end (String seq, String msg);
    void end (String msg, Post post);
    void end (String msg);
  };

  struct Timer {
    uv_timer_t handle;
    bool repeated = false;
    bool started = false;
    uint64_t timeout = 0;
    uint64_t interval = 0;
    uv_timer_cb invoke;
  };

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

  struct PeerRequestContext {
    uint64_t id;
    String seq;
    Callback cb;
    Peer* peer;
    char *buf;
    int bufsize;
    String address;
    int port;

    PeerRequestContext () {
      this->id = SSC::rand64();
    }

    PeerRequestContext (String s, Callback c)
      : PeerRequestContext(s, c, nullptr)
    {
      // noop
    }

    PeerRequestContext (String s, Callback c, Peer *p) {
      this->id = SSC::rand64();
      this->cb = c;
      this->seq = s;
      this->peer = p;
    }

    void end (String seq, String msg, Post post);
    void end (String seq, String msg);
    void end (String msg, Post post);
    void end (String msg);
  };

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
  struct Peer {
    // uv handles
    union {
      uv_udp_t udp;
      uv_tcp_t tcp; // XXX: FIXME
    } handle;

    // sockaddr
    struct sockaddr_in addr;

    // callbacks
    Callback recv;
    std::vector<std::function<void()>> onclose;

    // instance state
    uint64_t id = 0;
    std::recursive_mutex mutex;
    Core *core;

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
    Peer (Core *core, peer_type_t peerType, uint64_t peerId, bool isEphemeral);
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
    void send (String seq, char *buf, int len, int port, String address, Callback cb);
    int recvstart ();
    int recvstart (Callback onrecv);
    int recvstop ();
    int resume ();
    int pause ();
    void close ();
    void close (std::function<void()> onclose);
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

  class Core {
    public:
      std::unique_ptr<Tasks> tasks;
      std::unique_ptr<Posts> posts;
      std::map<uint64_t, Descriptor*> descriptors;
      std::map<uint64_t, Peer*> peers;

      std::recursive_mutex descriptorsMutex;
      std::recursive_mutex loopMutex;
      std::recursive_mutex peersMutex;
      std::recursive_mutex postsMutex;
      std::recursive_mutex tasksMutex;
      std::recursive_mutex timersMutex;

      std::atomic<bool> didLoopInit = false;
      std::atomic<bool> didTimersInit = false;
      std::atomic<bool> didTimersStart = false;

      std::atomic<bool> isLoopRunning = false;

      uv_loop_t eventLoop;
      uv_async_t eventLoopAsync;
      std::queue<EventLoopDispatchCallback> eventLoopDispatchQueue;

#if defined(__APPLE__)
      dispatch_queue_attr_t eventLoopQueueAttrs = dispatch_queue_attr_make_with_qos_class(
        DISPATCH_QUEUE_SERIAL,
        QOS_CLASS_DEFAULT,
        -1
      );

      dispatch_queue_t eventLoopQueue = dispatch_queue_create(
        "co.socketsupply.queue.event-loop",
        eventLoopQueueAttrs
      );
#else
      std::thread *eventLoopThread = nullptr;
#endif

      Core ();

      // fs
      static std::map<String, String> getFSConstantsMap ();
      String getFSConstants ();
      void fsAccess (String seq, String path, int mode, Callback cb);
      void fsChmod (String seq, String path, int mode, Callback cb);
      void fsCopyFile (String seq, String src, String dst, int mode, Callback cb);
      void fsClose (String seq, uint64_t id, Callback cb);
      void fsClosedir (String seq, uint64_t id, Callback cb);
      void fsCloseOpenDescriptor (String seq, uint64_t id, Callback cb);
      void fsCloseOpenDescriptors (String seq, Callback cb);
      void fsCloseOpenDescriptors (String seq, bool preserveRetained, Callback cb);
      void fsFStat (String seq, uint64_t id, Callback cb);
      void fsGetOpenDescriptors (String seq, Callback cb);
      void fsMkdir (String seq, String path, int mode, Callback cb);
      void fsOpen (String seq, uint64_t id, String path, int flags, int mode, Callback cb);
      void fsOpendir (String seq, uint64_t id, String path, Callback cb);
      void fsRead (String seq, uint64_t id, int len, int offset, Callback cb);
      void fsReaddir (String seq, uint64_t id, size_t entries, Callback cb);
      void fsRetainOpenDescriptor (String seq, uint64_t id, Callback cb);
      void fsRename (String seq, String pathA, String pathB, Callback cb);
      void fsRmdir (String seq, String path, Callback cb);
      void fsStat (String seq, String path, Callback cb);
      void fsUnlink (String seq, String path, Callback cb);
      void fsWrite (String seq, uint64_t id, String data, int64_t offset, Callback cb);

      Descriptor * getDescriptor (uint64_t id);
      void removeDescriptor (uint64_t id);
      bool hasDescriptor (uint64_t id);

      // udp
      void udpBind (String seq, uint64_t peerId, String address, int port, bool reuseAddr, Callback cb);
      void udpConnect (String seq, uint64_t peerId, String address, int port, Callback cb);
      void udpDisconnect (String seq, uint64_t peerId, Callback cb);
      void udpGetPeerName (String seq, uint64_t peerId, Callback cb);
      void udpGetSockName (String seq, uint64_t peerId, Callback cb);
      void udpGetState (String seq, uint64_t peerId,  Callback cb);
      void udpReadStart (String seq, uint64_t peerId, Callback cb);
      void udpReadStop (String seq, uint64_t peerId, Callback cb);
      void udpClose (String seq, uint64_t peerId, Callback cb);
      void udpSend (String seq, uint64_t peerId, char* buf, int len, int port, String address, bool ephemeral, Callback cb);

      void resumeAllPeers ();
      void pauseAllPeers ();
      bool hasPeer (uint64_t id);
      void removePeer (uint64_t id);
      void removePeer (uint64_t id, bool autoClose);
      Peer* getPeer (uint64_t id);
      Peer* createPeer (peer_type_t type, uint64_t id);
      Peer* createPeer (peer_type_t type, uint64_t id, bool isEphemeral);

      // core
      String getNetworkInterfaces ();
      void bufferSize (String seq, uint64_t peerId, int size, int buffer, Callback cb);
      void close (String seq, uint64_t peerId, Callback cb);
      void dnsLookup (String seq, String hostname, int family, Callback cb);
      void handleEvent (String seq, String event, String data, Callback cb);

      Task getTask (String id);
      bool hasTask (String id);
      void removeTask (String id);
      void putTask (String id, Task t);

      Post getPost (uint64_t id);
      bool hasPost (uint64_t id);
      void removePost (uint64_t id);
      void removeAllPosts ();
      void expirePosts ();
      void putPost (uint64_t id, Post p);
      String createPost (String seq, String params, Post post);

      // timers
      void initTimers ();
      void startTimers ();
      void stopTimers ();

      // loop
      uv_loop_t* getEventLoop ();
      int getEventLoopTimeout ();
      bool isLoopAlive ();
      void initEventLoop ();
      void runEventLoop ();
      void stopEventLoop ();
      void dispatchEventLoop (EventLoopDispatchCallback dispatch);
      void signalDispatchEventLoop ();
      void sleepEventLoop (int64_t ms);
      void sleepEventLoop ();
  };

} // SSC

#if defined(__APPLE__)
//
// Mixed-into ios.mm and mac.hh by #include. This file
// expects SSCBridgedWebView to be defined before it's included.
// All IO is routed though these common interfaces.
//
@class Bridge;

static dispatch_queue_attr_t qos = dispatch_queue_attr_make_with_qos_class(
  DISPATCH_QUEUE_CONCURRENT,
  QOS_CLASS_USER_INITIATED,
  -1
);

static dispatch_queue_t queue = dispatch_queue_create("co.socketsupply.queue.core", qos);

#if TARGET_OS_IPHONE || TARGET_IPHONE_SIMULATOR
@interface SSCBridgedWebView : WKWebView
@end
#else
@interface SSCBridgedWebView : WKWebView<
  WKUIDelegate,
  NSDraggingDestination,
  NSFilePromiseProviderDelegate,
  NSDraggingSource>
- (NSDragOperation) draggingSession: (NSDraggingSession *) session
sourceOperationMaskForDraggingContext: (NSDraggingContext) context;
@end
#endif

@interface BluetoothDelegate : NSObject<
  CBCentralManagerDelegate,
  CBPeripheralManagerDelegate,
  CBPeripheralDelegate>
@property (strong, nonatomic) Bridge* bridge;
@property (strong, nonatomic) CBCentralManager* centralManager;
@property (strong, nonatomic) CBPeripheralManager* peripheralManager;
@property (strong, nonatomic) CBPeripheral* bluetoothPeripheral;
@property (strong, nonatomic) NSMutableArray* peripherals;
@property (strong, nonatomic) NSMutableDictionary* services;
@property (strong, nonatomic) NSMutableDictionary* characteristics;
@property (strong, nonatomic) NSMutableDictionary* serviceMap;
- (void) publishCharacteristic: (SSC::String)seq buf: (char*)buf len: (int)len sid: (SSC::String)sid cid: (SSC::String)cid;
- (void) subscribeCharacteristic: (SSC::String)seq sid: (SSC::String)sid cid: (SSC::String)cid;
- (void) startService: (SSC::String)seq sid: (SSC::String)sid;
- (void) startAdvertising;
- (void) startScanning;
- (void) initBluetooth;
@end

@interface Bridge : NSObject
@property (strong, nonatomic) BluetoothDelegate* bluetooth;
@property (strong, nonatomic) SSCBridgedWebView* webview;
@property (nonatomic) SSC::Core* core;
@property nw_path_monitor_t monitor;
@property (strong, nonatomic) NSObject<OS_dispatch_queue>* monitorQueue;
- (bool) route: (SSC::String)msg buf: (char*)buf bufsize: (size_t)bufsize;
- (void) emit: (SSC::String)name msg: (SSC::String)msg;
- (void) send: (SSC::String)seq msg: (SSC::String)msg post: (SSC::Post)post;
- (void) setBluetooth: (BluetoothDelegate*)bd;
- (void) setWebview: (SSCBridgedWebView*)bv;
- (void) setCore: (SSC::Core*)core;
@end
#endif
#endif
