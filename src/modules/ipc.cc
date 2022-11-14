module;
#include "../core/ipc.hh"

/**
 * @module ssc.ipc
 * @description TODO
 */
export module ssc.ipc;
import ssc.data;
import ssc.javascript;
import ssc.json;
import ssc.network;
import ssc.string;
import ssc.types;
import ssc.utils;

using namespace ssc::javascript;
using namespace ssc::network;
using namespace ssc::utils;
using namespace ssc::types;

using ssc::data::Data;
using ssc::data::DataManager;

export namespace ssc::ipc {
  // forward
  class Router;

  using core::ipc::Message; // NOLINT(misc-unused-using-decls)
  using core::ipc::MessageBuffer; // NOLINT(misc-unused-using-decls)
  using core::ipc::Result; // NOLINT(misc-unused-using-decls)
  using Seq = Message::Seq; // NOLINT(misc-unused-using-decls)

  using EvaluateJavaScriptCallback = Function<void(const String)>;
  using DispatchFunction = Function<void()>;
  using DispatchCallback = Function<void(DispatchFunction)>;
  using ReplyCallback = Function<void(const Result&)>;
  using ResultCallback = Function<void(Result)>;
  using MessageCallback = Function<void(const Message, Router*, ReplyCallback)>;
  using BufferMap = std::map<String, MessageBuffer>;

  struct MessageCallbackContext {
    bool async = true;
    MessageCallback callback;
  };

  using RouteTable = std::map<String, MessageCallbackContext>;

  class Router {
    public:
      EvaluateJavaScriptCallback evaluateJavaScriptFunction = nullptr;
      DispatchCallback dispatchFunction = nullptr;

      NetworkStatusObserver networkStatusObserver;
      DataManager dataManager;
      RouteTable table;
      BufferMap buffers;
      Mutex mutex;

      Router (const Router &) = delete;
      Router () {
        this->networkStatusObserver = {
          [this](const String& statusName, const String& statusMessage) {
            this->onNetworkStatusChange(statusName, statusMessage);
          }
        };
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

      bool dispatch (DispatchFunction callback) {
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
          this->send(result.seq, result.json(), result.value.data);
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

      bool send (const Message::Seq& seq, const JSON::Any& json, Data& data) {
        if (data.body || seq == "-1") {
          auto script = this->dataManager.create(seq, json, data);
          return this->evaluateJavaScript(script);
        }

        // this had a sequence, we need to try to resolve it.
        if (seq != "-1" && seq.size() > 0) {
          auto value = encodeURIComponent(json.str());
          auto script = getResolveToRenderProcessJavaScript(seq, "0", value);
          return this->evaluateJavaScript(script);
        }

        if (JSON::typeof(json) == "string") {
          auto value = encodeURIComponent(json.str());
          return this->evaluateJavaScript(value);
        }

        return false;
      }
  };
}
