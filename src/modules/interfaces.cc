#include "../common.hh"

import :json;

export module ssc.runtime:interfaces;

export namespace ssc {
  struct Post {
    uint64_t id = 0;
    uint64_t ttl = 0;
    char* body = nullptr;
    size_t length = 0;
    String headers = "";
    bool bodyNeedsFree = false;
  };

  using Posts = std::map<uint64_t, Post>;
  using EventLoopDispatchCallback = std::function<void()>;

  class Runtime {
    public:
      class Module {
        public:
          using Callback = std::function<void(String, JSON::Any, Post)>;
          struct RequestContext {
            String seq;
            Module::Callback cb;
            RequestContext () = default;
            RequestContext (String seq, Module::Callback cb) {
              this->seq = seq;
              this->cb = cb;
            }
          };

          Runtime *runtime = nullptr;
          Module (Runtime* runtime) {
            this->runtime = runtime;
          }
      };

      class DNS : public Module {
        public:
          DNS (auto runtime) : Module(runtime) {}
          struct LookupOptions {
            String hostname;
            int family;
            // TODO: support these options
            // - hints
            // - all
            // -verbatim
          };
          void lookup (
            const String seq,
            LookupOptions options,
            Module::Callback cb
          );
      };

      class FS : public Module {
        public:
          FS (auto runtime) : Module(runtime) {}

          struct Descriptor {
            uint64_t id;
            std::atomic<bool> retained = false;
            std::atomic<bool> stale = false;
            Mutex mutex;
            uv_dir_t *dir = nullptr;
            uv_file fd = 0;
            Runtime *runtime;

            Descriptor (Runtime *runtime, uint64_t id);
            bool isDirectory ();
            bool isFile ();
            bool isRetained ();
            bool isStale ();
          };

          struct RequestContext : Module::RequestContext {
            uint64_t id;
            Descriptor *desc = nullptr;
            uv_fs_t req;
            uv_buf_t iov[16];
            // 256 which corresponds to DirectoryHandle.MAX_BUFFER_SIZE
            uv_dirent_t dirents[256];
            int offset = 0;
            int result = 0;

            RequestContext () = default;
            RequestContext (Descriptor *desc)
              : RequestContext(desc, "", nullptr) {}
            RequestContext (String seq, Callback cb)
              : RequestContext(nullptr, seq, cb) {}
            RequestContext (Descriptor *desc, String seq, Callback cb) {
              this->id = ssc::rand64();
              this->cb = cb;
              this->seq = seq;
              this->desc = desc;
              this->req.data = (void *) this;
            }

            ~RequestContext () {
              uv_fs_req_cleanup(&this->req);
            }

            void setBuffer (int index, size_t len, char *base);
            void freeBuffer (int index);
            char* getBuffer (int index);
            size_t getBufferSize (int index);
          };

          std::map<uint64_t, Descriptor*> descriptors;
          Mutex mutex;

          Descriptor * getDescriptor (uint64_t id);
          void removeDescriptor (uint64_t id);
          bool hasDescriptor (uint64_t id);

          void constants (const String seq, Module::Callback cb);
          void access (
            const String seq,
            const String path,
            int mode,
            Module::Callback cb
          );
          void chmod (
            const String seq,
            const String path,
            int mode,
            Module::Callback cb
          );
          void close (const String seq, uint64_t id, Module::Callback cb);
          void copyFile (
            const String seq,
            const String src,
            const String dst,
            int mode,
            Module::Callback cb
          );
          void closedir (const String seq, uint64_t id, Module::Callback cb);
          void closeOpenDescriptor (
            const String seq,
            uint64_t id,
            Module::Callback cb
          );
          void closeOpenDescriptors (const String seq, Module::Callback cb);
          void closeOpenDescriptors (
            const String seq,
            bool preserveRetained,
            Module::Callback cb
          );
          void fstat (const String seq, uint64_t id, Module::Callback cb);
          void getOpenDescriptors (const String seq, Module::Callback cb);
          void lstat (const String seq, const String path, Module::Callback cb);
          void mkdir (
            const String seq,
            const String path,
            int mode,
            Module::Callback cb
          );
          void open (
            const String seq,
            uint64_t id,
            const String path,
            int flags,
            int mode,
            Module::Callback cb
          );
          void opendir (
            const String seq,
            uint64_t id,
            const String path,
            Module::Callback cb
          );
          void read (
            const String seq,
            uint64_t id,
            size_t len,
            size_t offset,
            Module::Callback cb
          );
          void readdir (
            const String seq,
            uint64_t id,
            size_t entries,
            Module::Callback cb
          );
          void retainOpenDescriptor (
            const String seq,
            uint64_t id,
            Module::Callback cb
          );
          void rename (
            const String seq,
            const String src,
            const String dst,
            Module::Callback cb
          );
          void rmdir (
            const String seq,
            const String path,
            Module::Callback cb
          );
          void stat (
            const String seq,
            const String path,
            Module::Callback cb
          );
          void unlink (
            const String seq,
            const String path,
            Module::Callback cb
          );
          void write (
            const String seq,
            uint64_t id,
            char *bytes,
            size_t size,
            size_t offset,
            Module::Callback cb
          );
      };

      class OS : public Module {
        public:
          static const int RECV_BUFFER = 1;
          static const int SEND_BUFFER = 0;

          OS (auto runtime) : Module(runtime) {}
          void bufferSize (
            const String seq,
            uint64_t peerId,
            size_t size,
            int buffer,
            Module::Callback cb
          );
          void networkInterfaces (const String seq, Module::Callback cb) const;
      };

      class Platform : public Module {
        public:
          Platform (auto runtime) : Module(runtime) {}
          void event (
            const String seq,
            const String event,
            const String data,
            Module::Callback cb
          );
          void notify (
            const String seq,
            const String title,
            const String body,
            Module::Callback cb
          );
          void openExternal (
            const String seq,
            const String value,
            Module::Callback cb
          );
      };

      DNS dns;
      FS fs;
      OS os;
      Platform platform;
      UDP udp;

      std::shared_ptr<Posts> posts;
      std::map<uint64_t, Peer*> peers;

      std::recursive_mutex loopMutex;
      std::recursive_mutex peersMutex;
      std::recursive_mutex postsMutex;
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
        "co.socketsupply.queue.loop",
        eventLoopQueueAttrs
      );
#else
      std::thread *eventLoopThread = nullptr;
#endif

      Runtime () :
        dns(this),
        fs(this),
        os(this),
        platform(this),
        udp(this)
      {
        this->posts = std::shared_ptr<Posts>(new Posts());
        initEventLoop();
      }

      void resumeAllPeers ();
      void pauseAllPeers ();
      bool hasPeer (uint64_t id);
      void removePeer (uint64_t id);
      void removePeer (uint64_t id, bool autoClose);
      Peer* getPeer (uint64_t id);
      Peer* createPeer (peer_type_t type, uint64_t id);
      Peer* createPeer (peer_type_t type, uint64_t id, bool isEphemeral);

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
}
