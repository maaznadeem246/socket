module;
#include "../../core/platform.hh"

/**
 * @module ssc.ipc.result;
 * @description TODO
 * @example TODO
 */
export module ssc.ipc.router;
import ssc.ipc.message;
import ssc.ipc.result;

import ssc.javascript;
import ssc.network;
import ssc.runtime;
import ssc.string;
import ssc.types;
import ssc.utils;
import ssc.codec;
import ssc.json;

using ssc::ipc::message::MessageBuffer;
using ssc::ipc::message::Message;
using ssc::ipc::result::Result;

using ssc::javascript::getResolveToRenderProcessJavaScript;
using ssc::javascript::getEmitToRenderProcessJavaScript;
using ssc::network::NetworkStatusObserver;
using ssc::codec::encodeURIComponent;
using ssc::runtime::Runtime;
using ssc::string::String;
using ssc::string::trim;
using ssc::utils::rand64;
using ssc::types::Vector;
using ssc::types::Mutex;
using ssc::types::Posts;
using ssc::types::Post;
using ssc::types::Lock;

export namespace ssc::ipc::router {
  // forward
  class Router;

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

  using RouteTable = std::map<String, MessageCallbackContext>;

  class Router {
    public:
      EvaluateJavaScriptCallback evaluateJavaScriptFunction = nullptr;
      std::function<void(DispatchCallback)> dispatchFunction = nullptr;
      NetworkStatusObserver networkStatusObserver;
      BufferMap buffers;
      Mutex mutex;

      RouteTable table;
      Runtime *runtime = nullptr;

      std::shared_ptr<Posts> posts;

      Router (const Router &) = delete;
      Router () {
        this->posts = std::shared_ptr<Posts>(new Posts());

        this->networkStatusObserver = NetworkStatusObserver([this](const String& statusName, const String& statusMessage) {
          this->onNetworkStatusChange(statusName, statusMessage);
        });
      }

      ~Router () {
      }

      void onNetworkStatusChange (
        const String& statusName,
        const String& statusMessage
      ) {
        auto json = JSON::Object::Entries {
          {"status", statusName},
          {"message", statusMessage}
        };

        this->emit(statusName, json);
      }

      auto hasMappedBuffer (int index, const Message::Seq& seq) {
        Lock lock(this->mutex);
        auto key = std::to_string(index) + seq;
        return this->buffers.find(key) != this->buffers.end();
      }

      auto getMappedBuffer (int index, const Message::Seq& seq) {
        if (this->hasMappedBuffer(index, seq)) {
          Lock lock(this->mutex);
          auto key = std::to_string(index) + seq;
          return this->buffers.at(key);
        }

        return MessageBuffer {};
      }

      auto removeMappedBuffer (int index, const Message::Seq& seq) {
        Lock lock(this->mutex);
        if (this->hasMappedBuffer(index, seq)) {
          auto key = std::to_string(index) + seq;
          this->buffers.erase(key);
        }
      }

      auto setMappedBuffer (
        int index,
        const Message::Seq& seq,
        char* bytes,
        size_t size
      ) {
        Lock lock(this->mutex);
        auto key = std::to_string(index) + seq;
        this->buffers.insert_or_assign(key, MessageBuffer { bytes, size });
      }

      void map (const String& name, MessageCallback callback) {
        return this->map(name, true, callback);
      }

      void map (const String& name, bool async, MessageCallback callback) {
        if (callback != nullptr) {
          table.insert_or_assign(name, MessageCallbackContext { async, callback });
        }
      }

      void unmap (const String& name) {
        if (table.find(name) != table.end()) {
          table.erase(name);
        }
      }

      bool dispatch (DispatchCallback callback) {
        if (this->dispatchFunction != nullptr) {
          this->dispatchFunction(callback);
          return true;
        }

        return false;
      }

      bool emit (
        const String& name,
        const String& data
      ) {
        auto value = encodeURIComponent(data);
        auto script = getEmitToRenderProcessJavaScript(name, value);
        return this->evaluateJavaScript(script);
      }

      bool emit (
        const String& name,
        const JSON::Any& data
      ) {
        return this->emit(name, data.str());
      }

      bool evaluateJavaScript (const String& js) {
        if (this->evaluateJavaScriptFunction != nullptr) {
          this->evaluateJavaScriptFunction(js);
          return true;
        }

        return false;
      }

      bool invoke (const Message& message) {
        return this->invoke(message, [this](auto result) {
          this->send(result.seq, result.str(), result.post);
        });
      }

      bool invoke (const Message& message, ResultCallback callback) {
        if (this->table.find(message.name) == this->table.end()) {
          return false;
        }

        auto ctx = this->table.at(message.name);

        if (ctx.callback != nullptr) {
          Message msg(message);
          // decorate message with buffer if buffer was previously
          // mapped with `ipc://buffer.map`, which we do on Linux
          if (this->hasMappedBuffer(msg.index, msg.seq)) {
            msg.buffer = this->getMappedBuffer(msg.index, msg.seq);
            this->removeMappedBuffer(msg.index, msg.seq);
          }

          if (ctx.async) {
            return this->dispatch([ctx, msg, callback, this] {
              ctx.callback(msg, this, callback);
            });
          } else {
            ctx.callback(msg, this, callback);
            return true;
          }
        }

        return false;
      }

      bool invoke (const String& name, char *bytes, size_t size) {
        auto message = Message { name, bytes, size };
        return this->invoke(message);
      }

      bool invoke (
        const String& name,
        char *bytes,
        size_t size,
        ResultCallback callback
      ) {
        auto message = Message { name, bytes, size };
        return this->invoke(message, callback);
      }

      bool send (const Message::Seq& seq, const String& data, Post& post) {
        if (post.body || seq == "-1") {
          auto script = this->createPost(seq, data, post);
          return this->evaluateJavaScript(script);
        }

        // this had a sequence, we need to try to resolve it.
        if (seq != "-1" && seq.size() > 0) {
          auto value = encodeURIComponent(data);
          auto script = getResolveToRenderProcessJavaScript(seq, "0", value);
          return this->evaluateJavaScript(script);
        }

        if (data.size() > 0) {
          return this->evaluateJavaScript(data);
        }

        return false;
      }

      Post getPost (uint64_t id) {
        Lock lock(this->mutex);
        if (this->posts->find(id) == this->posts->end()) return Post{};
        return this->posts->at(id);
      }

      bool hasPost (uint64_t id) {
        Lock lock(this->mutex);
        return this->posts->find(id) != this->posts->end();
      }

      void expirePosts () {
        Lock lock(this->mutex);
        std::vector<uint64_t> ids;
        auto now = std::chrono::system_clock::now()
          .time_since_epoch()
          .count();

        for (auto const &tuple : *this->posts) {
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

      void putPost (uint64_t id, Post& post) {
        Lock lock(this->mutex);
        post.ttl = std::chrono::time_point_cast<std::chrono::milliseconds>(
          std::chrono::system_clock::now() +
          std::chrono::milliseconds(32 * 1024)
        )
          .time_since_epoch()
          .count();

        this->posts->insert_or_assign(id, post);
      }

      void removePost (uint64_t id) {
        Lock lock(this->mutex);
        if (posts->find(id) == posts->end()) return;
        auto post = getPost(id);

        if (post.body && post.bodyNeedsFree) {
          delete [] post.body;
          post.bodyNeedsFree = false;
          post.body = nullptr;
        }

        this->posts->erase(id);
      }

      String createPost (String seq, String params, Post& post) {
        Lock lock(this->mutex);

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

      void removeAllPosts () {
        Lock lock(this->mutex);
        Vector<uint64_t> ids;

        for (auto const &tuple : *posts) {
          auto id = tuple.first;
          ids.push_back(id);
        }

        for (auto const id : ids) {
          removePost(id);
        }
      }
  };
}
