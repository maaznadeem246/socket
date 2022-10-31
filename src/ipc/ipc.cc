#include "../runtime/runtime.hh"
#include "ipc.hh"

#include "../core/core.hh"

namespace ssc::ipc {
  class Router;
  class Bridge;
}

#if defined(__APPLE__)
namespace ssc::ipc {
  using Task = id<WKURLSchemeTask>;
  using Tasks = std::map<String, Task>;
}

@class SSCBridgedWebView;
@interface SSCIPCSchemeHandler : NSObject<WKURLSchemeHandler>
@property (nonatomic) ssc::ipc::Router* router;
-     (void) webView: (SSCBridgedWebView*) webview
  startURLSchemeTask: (ssc::ipc::Task) task;
-    (void) webView: (SSCBridgedWebView*) webview
  stopURLSchemeTask: (ssc::ipc::Task) task;
@end

@interface SSCIPCSchemeTasks : NSObject {
  std::unique_ptr<ssc::ipc::Tasks> tasks;
  ssc::Mutex mutex;
}
- (ssc::ipc::Task) get: (ssc::String) id;
- (void) remove: (ssc::String) id;
- (bool) has: (ssc::String) id;
- (void) put: (ssc::String) id task: (ssc::ipc::Task) task;
@end

#if defined(__APPLE__)
@interface SSCIPCNetworkStatusObserver : NSObject
@property (strong, nonatomic) NSObject<OS_dispatch_queue>* monitorQueue;
@property (nonatomic) ssc::ipc::Router* router;
@property (retain) nw_path_monitor_t monitor;
- (id) init;
@end
#endif
#endif

export module ssc.ipc;

export namespace ssc::ipc {
  struct MessageBuffer {
    char *bytes = nullptr;
    size_t size = 0;
    MessageBuffer () = default;
    MessageBuffer (auto bytes, auto size) {
      this->bytes = bytes;
      this->size = size;
    }
  };

  class Message {
    public:
      using Seq = String;
      MessageBuffer buffer;
      String value = "";
      String name = "";
      String seq = "";
      String uri = "";
      int index = -1;
      Map args;

      Message () = default;
      Message (const Message& message);
      Message (const String& source);
      Message (const String& source, char *bytes, size_t size);
      bool has (const String& key) const;
      String get (const String& key) const;
      String get (const String& key, const String& fallback) const;
      String str () const { return this->uri; }
      const char * c_str () const { return this->uri.c_str(); }
  };

  class Result {
    public:
      class Err {
        public:
          Message message;
          Message::Seq seq;
          JSON::Any value;
          Err () = default;
          Err (const Message&, JSON::Any);
      };

      class Data {
        public:
          Message message;
          Message::Seq seq;
          JSON::Any value;
          Post post;

          Data () = default;
          Data (const Message&, JSON::Any);
          Data (const Message&, JSON::Any, Post);
      };

      Message message;
      Message::Seq seq;
      String source = "";
      JSON::Any value = nullptr;
      JSON::Any data = nullptr;
      JSON::Any err = nullptr;
      Post post;

      Result ();
      Result (const Err error);
      Result (const Data data);
      Result (const Message::Seq&, const Message&);
      Result (const Message::Seq&, const Message&, JSON::Any);
      Result (const Message::Seq&, const Message&, JSON::Any, Post);
      String str () const;
      JSON::Any json () const;
  };

  class Router {
    public:
      using EvaluateJavaScriptCallback = std::function<void(const String)>;
      using DispatchCallback = std::function<void()>;
      using ReplyCallback = std::function<void(const Result&)>;
      using ResultCallback = std::function<void(Result)>;
      using MessageCallback = std::function<void(const Message, Router*, ReplyCallback)>;
      using BufferMap = std::map<String, MessageBuffer>;

      struct MessageCallbackContext {
        bool async = true;
        MessageCallback callback;
      };

      using Table = std::map<String, MessageCallbackContext>;

      EvaluateJavaScriptCallback evaluateJavaScriptFunction = nullptr;
      std::function<void(DispatchCallback)> dispatchFunction = nullptr;
      BufferMap buffers;
      Mutex mutex;
      Table table;
      Core *core = nullptr;
      Bridge *bridge = nullptr;
      std::shared_ptr<Posts> posts;
      Mutex postsMutex;
#if defined(__APPLE__)
      SSCIPCNetworkStatusObserver* networkStatusObserver = nullptr;
      SSCIPCSchemeHandler* schemeHandler = nullptr;
      SSCIPCSchemeTasks* schemeTasks = nullptr;
#endif

      Router ();
      Router (const Router &) = delete;
      ~Router ();

      MessageBuffer getMappedBuffer (int index, const Message::Seq seq);
      bool hasMappedBuffer (int index, const Message::Seq seq);
      void removeMappedBuffer (int index, const Message::Seq seq);
      void setMappedBuffer (
        int index,
        const Message::Seq seq,
        char* bytes,
        size_t size
      );

      void map (const String& name, MessageCallback callback);
      void map (const String& name, bool async, MessageCallback callback);
      void unmap (const String& name);
      bool dispatch (DispatchCallback callback);
      bool emit (const String& name, const String& data);
      bool evaluateJavaScript (const String javaScript);
      bool invoke (const Message& message);
      bool invoke (const Message& message, ResultCallback callback);
      bool invoke (const String& msg, char *bytes, size_t size);
      bool invoke (
        const String& msg,
        char *bytes,
        size_t size,
        ResultCallback callback
      );
      bool send (const Message::Seq& seq, const String& data, const Post post);
  };

  Post Router::getPost (uint64_t id) {
    Lock lock(postsMutex);
    if (posts->find(id) == posts->end()) return Post{};
    return posts->at(id);
  }

  bool Router::hasPost (uint64_t id) {
    Lock lock(postsMutex);
    return posts->find(id) != posts->end();
  }

  void Router::expirePosts () {
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

  void Router::putPost (uint64_t id, Post p) {
    Lock lock(postsMutex);
    p.ttl = std::chrono::time_point_cast<std::chrono::milliseconds>(
      std::chrono::system_clock::now() +
      std::chrono::milliseconds(32 * 1024)
    )
      .time_since_epoch()
      .count();
    posts->insert_or_assign(id, p);
  }

  void Router::removePost (uint64_t id) {
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

  String Router::createPost (String seq, String params, Post post) {
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

  void Router::removeAllPosts () {
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

  class Bridge {
    public:
      Router router;
      Bluetooth bluetooth;
      Core *core = nullptr;

      Bridge (Core *core);
      bool route (const String& msg, char *bytes, size_t size);
  };

  inline String getResolveToMainProcessMessage (
    const String& seq,
    const String& state,
    const String& value
  ) {
    return String("ipc://resolve?seq=" + seq + "&state=" + state + "&value=" + value);
  }
} // ssc::IPC

namespace ssc::ipc {
  Message::Message (const Message& message) {
    this->buffer.bytes = message.buffer.bytes;
    this->buffer.size = message.buffer.size;
    this->value = message.value;
    this->index = message.index;
    this->name = message.name;
    this->seq = message.seq;
    this->uri = message.uri;
    this->args = message.args;
  }

  Message::Message (const String& source, char *bytes, size_t size)
    : Message(source)
  {
    this->buffer.bytes = bytes;
    this->buffer.size = size;
  }

  Message::Message (const String& source) {
    String str = source;
    uri = str;

    // bail if missing protocol prefix
    if (str.find("ipc://") == -1) return;

    // bail if malformed
    if (str.compare("ipc://") == 0) return;
    if (str.compare("ipc://?") == 0) return;

    String query;
    String path;

    auto raw = split(str, '?');
    path = raw[0];
    if (raw.size() > 1) query = raw[1];

    auto parts = split(path, '/');
    if (parts.size() >= 1) name = parts[1];

    if (raw.size() != 2) return;
    auto pairs = split(raw[1], '&');

    for (auto& rawPair : pairs) {
      auto pair = split(rawPair, '=');
      if (pair.size() <= 1) continue;

      if (pair[0].compare("index") == 0) {
        try {
          index = std::stoi(pair[1].size() > 0 ? pair[1] : "0");
        } catch (...) {
          std::cout << "Warning: received non-integer index" << std::endl;
        }
      }

      if (pair[0].compare("value") == 0) {
        value = decodeURIComponent(pair[1]);
      }

      if (pair[0].compare("seq") == 0) {
        seq = decodeURIComponent(pair[1]);
      }

      args[pair[0]] = pair[1];
    }
  }

  bool Message::has (const String& key) const {
    return this->args.find(key) != this->args.end();
  }

  String Message::get (const String& key) const {
    return this->get(key, "");
  }

  String Message::get (const String& key, const String &fallback) const {
    return args.count(key) ? decodeURIComponent(args.at(key)) : fallback;
  }

  Result::Result () {
  }

  Result::Result (
    const Message::Seq& seq,
    const Message& message
  ) {
    this->message = message;
    this->source = message.name;
    this->seq = seq;
  }

  Result::Result (
    const Message::Seq& seq,
    const Message& message,
    JSON::Any value
  ) : Result(seq, message, value, Post{}) {
  }

  Result::Result (
    const Message::Seq& seq,
    const Message& message,
    JSON::Any value,
    Post post
  ) : Result(seq, message) {
    this->post = post;

    if (value.type != JSON::Type::Any) {
      this->value = value;
    }
  }

  Result::Result (const Err error) {
    this->err = error.value;
  }

  Result::Result (const Data data) {
    this->data = data.value;
  }

  String Result::str () const {
    if (this->value.type != JSON::Type::Null) {
      return this->value.str();
    }

    auto entries = JSON::Object::Entries {
      {"source", this->source},
      {"data", this->data},
      {"err", this->err}
    };

    return JSON::Object(entries).str();
  }

  Result::Err::Err (
    const Message& message,
    JSON::Any value
  ) {
    this->seq = message.seq;
    this->message = message;
    this->value = value;
  }

  Result::Data::Data (
    const Message& message,
    JSON::Any value
  ) : Data(message, value, Post{}) {
  }

  Result::Data::Data (
    const Message& message,
    JSON::Any value,
    Post post
  ) {
    this->seq = message.seq;
    this->message = message;
    this->value = value;
    this->post = post;
  }
}
