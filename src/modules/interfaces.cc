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
