module; // global
#include "../platform.hh"

/**
 * @module runtime
 * @description TODO
 * @example
 */
export module runtime;
import javascript;
import uv;

export namespace ssc::runtime {
  using EventLoopDispatchCallback = std::function<void()>;

  class Peer;
  class Runtime {
    public:

      std::shared_ptr<Posts> posts;
      std::map<uint64_t, Peer*> peers;

      Mutex loopMutex;
      Mutex peersMutex;
      Mutex postsMutex;
      Mutex timersMutex;

      std::atomic<bool> didLoopInit = false;
      std::atomic<bool> didTimersInit = false;
      std::atomic<bool> didTimersStart = false;

      std::atomic<bool> isLoopRunning = false;

      uv_loop_t eventLoop;
      uv_async_t eventLoopAsync;
      Queue<EventLoopDispatchCallback> eventLoopDispatchQueue;

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

      Runtime () {
        this->posts = std::shared_ptr<Posts>(new Posts());
        initEventLoop();
      }

      Post getPost (uint64_t id);
      bool hasPost (uint64_t id);
      void removePost (uint64_t id);
      void removeAllPosts ();
      void expirePosts ();
      void putPost (uint64_t id, Post p);
      String createPost (String seq, String params, Post post);

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

  Post Runtime::getPost (uint64_t id) {
    Lock lock(postsMutex);
    if (posts->find(id) == posts->end()) return Post{};
    return posts->at(id);
  }

  bool Runtime::hasPost (uint64_t id) {
    Lock lock(postsMutex);
    return posts->find(id) != posts->end();
  }

  void Runtime::expirePosts () {
    Lock lock(postsMutex);
    std::vector<uint64_t> ids;
    auto now = std::chrono::system_clock::now()
      .time_since_epoch()
      .count();

    for (auto const &tuple : *posts) {
      auto id = tuple.first;
      auto post = tuple.second;

      if (post.ttl < now) {
        ids.push_back(id);
      }
    }

    for (auto const id : ids) {
      removePost(id);
    }
  }

  void Runtime::putPost (uint64_t id, Post p) {
    Lock lock(postsMutex);
    p.ttl = std::chrono::time_point_cast<std::chrono::milliseconds>(
      std::chrono::system_clock::now() +
      std::chrono::milliseconds(32 * 1024)
    )
      .time_since_epoch()
      .count();
    posts->insert_or_assign(id, p);
  }

  void Runtime::removePost (uint64_t id) {
    Lock lock(postsMutex);
    if (posts->find(id) == posts->end()) return;
    auto post = getPost(id);

    if (post.body && post.bodyNeedsFree) {
      delete [] post.body;
      post.bodyNeedsFree = false;
      post.body = nullptr;
    }

    posts->erase(id);
  }

  String Runtime::createPost (String seq, String params, Post post) {
    Lock lock(postsMutex);

    if (post.id == 0) {
      post.id = rand64();
    }

    auto sid = std::to_string(post.id);
    auto js = javascript::createJavaScript("post-data.js",
      "const xhr = new XMLHttpRequest();                             \n"
      "xhr.responseType = 'arraybuffer';                             \n"
      "xhr.onload = e => {                                           \n"
      "  let params = `" + params + "`;                              \n"
      "  params.seq = `" + seq + "`;                                 \n"
      "                                                              \n"
      "  try {                                                       \n"
      "    params = JSON.parse(params);                              \n"
      "  } catch (err) {                                             \n"
      "    console.error(err.stack || err, params);                  \n"
      "  };                                                          \n"
      "                                                              \n"
      "  const headers = `" + trim(post.headers) + "`                \n"
      "    .trim()                                                   \n"
      "    .split(/[\\r\\n]+/)                                       \n"
      "    .filter(Boolean);                                         \n"
      "                                                              \n"
      "  const detail = {                                            \n"
      "    data: xhr.response,                                       \n"
      "    sid: '" + sid + "',                                       \n"
      "    headers: Object.fromEntries(                              \n"
      "      headers.map(l => l.split(/\\s*:\\s*/))                  \n"
      "    ),                                                        \n"
      "    params: params                                            \n"
      "  };                                                          \n"
      "                                                              \n"
      "  queueMicrotask(() => {                                      \n"
      "    const event = new window.CustomEvent('data', { detail }); \n"
      "    window.dispatchEvent(event);                              \n"
      "  });                                                         \n"
      "};                                                            \n"
      "                                                              \n"
      "xhr.open('GET', 'ipc://post?id=" + sid + "');                 \n"
      "xhr.send();                                                   \n"
    );

    putPost(post.id, post);
    return js;
  }

  void Runtime::removeAllPosts () {
    Lock lock(postsMutex);
    std::vector<uint64_t> ids;

    for (auto const &tuple : *posts) {
      auto id = tuple.first;
      ids.push_back(id);
    }

    for (auto const id : ids) {
      removePost(id);
    }
  }
}
