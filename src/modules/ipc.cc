module;
#include "../core/ipc.hh"
#include "../core/network.hh"
#include <new>

/**
 * @module ssc.ipc
 * @description TODO
 */
export module ssc.ipc;
import ssc.data;
import ssc.javascript;
import ssc.json;
import ssc.runtime;
import ssc.string;
import ssc.types;
import ssc.utils;
import ssc.log;

using ssc::core::network::NetworkStatusObserver;

using namespace ssc::data;
using namespace ssc::javascript;
using namespace ssc::types;
using namespace ssc::utils;

using ssc::runtime::Runtime;

export namespace ssc::ipc {
  // forward
  class Router;

  using core::ipc::IRouter;
  using core::ipc::Message; // NOLINT(misc-unused-using-decls)
  using core::ipc::MessageBuffer; // NOLINT(misc-unused-using-decls)
  using core::ipc::Result; // NOLINT(misc-unused-using-decls)
  using Seq = Message::Seq; // NOLINT(misc-unused-using-decls)

  using EvaluateJavaScriptCallback = Function<void(const String)>;
  using BufferMap = std::map<String, MessageBuffer>;

  using DispatchCallback = IRouter::DispatchCallback;
  using ResultCallback = IRouter::ResultCallback;
  using ReplyCallback = IRouter::ReplyCallback;
  using RouteCallback = IRouter::RouteCallback;

  struct RouteCallbackContext {
    bool async = true;
    RouteCallback callback;
  };

  using RouteTable = std::map<String, RouteCallbackContext>;

  class Router : public IRouter {
    public:
      EvaluateJavaScriptCallback evaluateJavaScriptFunction = nullptr;

      NetworkStatusObserver networkStatusObserver;
      Runtime& runtime;
      RouteTable table;
      BufferMap buffers;
      Mutex mutex;

      Router (const Router &) = delete;
      Router (Runtime& runtime) : runtime(runtime) {
        this->networkStatusObserver.onNetworkStatusChangeCallback =
          [&](const String& statusName, const String& statusMessage) {
            this->onNetworkStatusChange(statusName, statusMessage);
          };
      }

      ~Router () {
      }

      void onNetworkStatusChange (
        const String statusName,
        const String statusMessage
      ) {
        auto json = JSON::Object::Entries {
          {"status", statusName},
          {"message", statusMessage}
        };

        this->emit(statusName, json);
      }

      bool hasMappedBuffer (int index, const Message::Seq& seq) {
        Lock lock(this->mutex);
        auto key = std::to_string(index) + seq;
        return this->buffers.find(key) != this->buffers.end();
      }

      MessageBuffer getMappedBuffer (int index, const Message::Seq& seq) {
        if (this->hasMappedBuffer(index, seq)) {
          Lock lock(this->mutex);
          auto key = std::to_string(index) + seq;
          return this->buffers.at(key);
        }

        return MessageBuffer {};
      }

      void removeMappedBuffer (int index, const Message::Seq& seq) {
        Lock lock(this->mutex);
        if (this->hasMappedBuffer(index, seq)) {
          auto key = std::to_string(index) + seq;
          this->buffers.erase(key);
        }
      }

      void setMappedBuffer (
        int index,
        const Message::Seq& seq,
        char* bytes,
        size_t size
      ) {
        Lock lock(this->mutex);
        auto key = std::to_string(index) + seq;
        this->buffers.insert_or_assign(key, MessageBuffer { bytes, size });
      }

      void map (const String& name, RouteCallback callback) {
        return this->map(name, true, callback);
      }

      void map (const String& name, bool async, RouteCallback callback) {
        if (callback != nullptr) {
          table.insert_or_assign(name, RouteCallbackContext { async, callback });
        }
      }

      void unmap (const String& name) {
        if (table.find(name) != table.end()) {
          table.erase(name);
        }
      }

      bool dispatch (DispatchFunction callback) {
        this->runtime.dispatch(callback);
        return true;
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

      bool send (const Message::Seq& seq, const JSON::Any& json, Data data) {
        if (data.body || seq == "-1") {
          auto script = this->runtime.dataManager.create(seq, json, data);
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
