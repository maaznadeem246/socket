module;
#include <socket/platform.hh>
#include <functional>
#include <map>
#include <string>

/**
 * @module ssc.ipc.result;
 * @description TODO
 * @example TODO
 */
export module ssc.router;
import ssc.ipc.message;
import ssc.ipc.result;
import ssc.ipc.data;

import ssc.javascript;
import ssc.network;
import ssc.string;
import ssc.webview;
import ssc.types;
import ssc.utils;
import ssc.codec;
import ssc.json;

using ssc::ipc::data::Data;
using ssc::ipc::data::DataManager;
using ssc::ipc::message::Message;
using ssc::ipc::message::MessageBuffer;
using ssc::ipc::result::Result;

using ssc::javascript::getEmitToRenderProcessJavaScript;
using ssc::javascript::getResolveToRenderProcessJavaScript;

using ssc::codec::encodeURIComponent;
using ssc::network::NetworkStatusObserver;

using ssc::string::String;

using ssc::types::Lock;
using ssc::types::Mutex;
using ssc::types::SharedPointer;
using ssc::types::Vector;

using ssc::webview::IPCSchemeHandler;

export namespace ssc::router {
  // forward
  class Router;

  using EvaluateJavaScriptCallback = std::function<void(const String)>;
  using DispatchFunction = std::function<void()>;
  using DispatchCallback = std::function<void(DispatchFunction)>;
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
      DispatchCallback dispatchFunction = nullptr;

      NetworkStatusObserver networkStatusObserver;
      IPCSchemeHandler schemeHandler;
      DataManager dataManager;
      RouteTable table;
      BufferMap buffers;
      Mutex mutex;

      Router (const Router &) = delete;
      Router () {
        this->schemeHandler = {
          &this->dataManager,
          [this](const auto& request) {
            auto message = Message { request.url };
            message.buffer.bytes = request.body.bytes;
            message.buffer.size = request.body.size;
            return this->invoke(message);
          }
        };

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
