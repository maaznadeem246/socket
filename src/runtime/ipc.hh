#ifndef SSC_RUNTIME_IPC_HH
#define SSC_RUNTIME_IPC_HH

#include <socket/socket.hh>
#include "runtime.hh"
#include "utils.hh"

namespace ssc::runtime::ipc {
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
      int index = -1;
      Map args;
      String string;

      Message () = default;
      Message (const Message& message);
      Message (const String& source);
      Message (const String& source, char *bytes, size_t size);

      bool has (const String& key) const;
      String get (const String& key) const;
      void set (const String& key, const String& value);
      String get (const String& key, const String& fallback) const;
      String str () const;
      size_t size () const;

      String operator [] (const String& key) const;
      String &operator [] (const String& key);
  };

  struct Value {
    runtime::Data data;
    struct {
      JSON::Any data;
      JSON::Any err;
      JSON::Any raw;
    } json;
  };

  class Result {
    public:
      class Err {
        public:
          Message message;
          Message::Seq seq;
          JSON::Any json;
          Err () = default;
          Err (const Message& message, const JSON::Any& json);
      };

      class Data {
        public:
          Message message;
          Message::Seq seq;
          JSON::Any json;

          Data () = default;
          Data (const Message& message, const JSON::Any& json);
      };

      Message message;
      Message::Seq seq;
      String source = "";
      Value value;

      Result () = default;
      Result (const Err& err);
      Result (const Data& data);
      Result (const Message::Seq& seq, const Message& message);
      Result (const Message::Seq& seq, const Message& message, JSON::Any json);
      Result (
        const Message::Seq& seq,
        const Message& message,
        JSON::Any json,
        runtime::Data data
      );

      String str () const;
      runtime::Data data () const;
      JSON::Any json () const;
  };

  class Router {
    public:
      using EvaluateJavaScriptCallback = Function<void(const String)>;
      using DispatchFunction = Function<void()>;
      using DispatchCallback = Function<void(DispatchFunction)>;
      using ResultCallback = Function<void(Result)>;
      using ReplyCallback = Function<void(const Result&)>;
      using RouteCallback = Function<void(const Message, Router*, ReplyCallback)>;
      using BufferMap = std::map<String, MessageBuffer>;

      struct RouteCallbackContext {
        bool async = true;
        RouteCallback callback;
      };

      using RouteTable = std::map<String, RouteCallbackContext>;

      EvaluateJavaScriptCallback evaluateJavaScriptFunction = nullptr;

      NetworkStatusObserver networkStatusObserver;
      Runtime& runtime;
      RouteTable table;
      BufferMap buffers;
      Mutex mutex;

      Router (const Router &) = delete;
      Router (Runtime& runtime);
      ~Router ();

      void onNetworkStatusChange (const String status, const String message);

      bool hasMappedBuffer (int index, const Message::Seq& seq);
      MessageBuffer getMappedBuffer (int index, const Message::Seq& seq);
      void removeMappedBuffer (int index, const Message::Seq& seq);
      void setMappedBuffer (
        int index,
        const Message::Seq& seq,
        char* bytes,
        size_t size
      );

      bool evaluateJavaScript (const String& js);
      void map (const String& name, RouteCallback callback);
      void map (const String& name, bool async, RouteCallback callback);
      void unmap (const String& name);
      bool dispatch (DispatchFunction callback);
      bool emit (const String& name, const String& data);
      bool emit (const String& name, const JSON::Any& data);

      bool send (const Message::Seq& seq, const JSON::Any& json, runtime::Data data);
      bool invoke (const Message& message);
      bool invoke (const Message& message, ResultCallback callback);
      bool invoke (const String& name, char *bytes, size_t size);
      bool invoke (
        const String& name,
        char *bytes,
        size_t size,
        ResultCallback callback
      );
  };

  class Bridge {
    public:
      Router router;
      Runtime& runtime;
      Bluetooth bluetooth;
      PlatformInfo platform;

      Bridge (const Bridge&) = delete;
      Bridge (Runtime& runtime)
        : runtime(runtime),
          router(runtime),
          bluetooth(&router)
      {
        this->init();
      }

      void init ();
  };
}
#endif
