#include <socket/runtime.hh>
#include <socket/utils.hh>

using namespace ssc::utils;

namespace ssc::runtime::ipc {
  Message::Message (const Message& message) {
    this->buffer.bytes = message.buffer.bytes;
    this->buffer.size = message.buffer.size;
    this->value = message.value;
    this->index = message.index;
    this->name = message.name;
    this->seq = message.seq;
    this->args = message.args;
  }

  Message::Message (const String& source) {
    String str = source;

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
          this->index = std::stoi(pair[1].size() > 0 ? pair[1] : "0");
        } catch (...) {
          fprintf(stderr, "%s\n", "Warning: received non-integer index");
          continue;
        }
      }

      if (pair[0].compare("value") == 0) {
        this->value = decodeURIComponent(pair[1]);
      }

      if (pair[0].compare("seq") == 0) {
        this->seq = decodeURIComponent(pair[1]);
      }

      args[pair[0]] = pair[1];
    }
  }

  Message::Message (const String& source, char *bytes, size_t size)
    : Message(source)
  {
    this->buffer.bytes = bytes;
    this->buffer.size = size;
  }

  bool Message::has (const String& key) const {
    return this->args.find(key) != this->args.end();
  }

  String Message::get (const String& key) const {
    return this->get(key, "");
  }

  void Message::set (const String& key, const String& value) {
    this->args.insert_or_assign(key, value);
  }

  String Message::get (const String& key, const String& fallback) const {
    return args.count(key) ? decodeURIComponent(args.at(key)) : fallback;
  }

  String Message::str () const {
    StringStream stream;

    stream << "ipc://" << this->name << "?";
    for (const auto& entry : this->args) {
      stream << encodeURIComponent(entry.first);
      stream << "=";
      stream << encodeURIComponent(entry.second);
      stream << "&";
    }

    return stream.str();
  }

  String Message::operator [] (const String& key) const {
    for (const auto& entry : this->args) {
      if (entry.first == key) {
        return entry.second;
      }
    }

    return "";
  }

  String& Message::operator [] (const String& key) {
    static String empty = "";

    if (!this->has(key)) {
      this->set(key, "");
    }

    for (auto& entry : this->args) {
      if (entry.first == key) {
        return entry.second;
      }
    }

    return empty;
  }

  size_t Message::size () const {
    return this->args.size();
  }

  Result::Err::Err (const Message& message, const JSON::Any& json) {
    this->seq = message.seq;
    this->message = message;
    this->json = json;
  }

  Result::Data::Data (const Message& message, const JSON::Any& json) {
    this->seq = message.seq;
    this->message = message;
    this->json = json;
  }

  Result::Result (const Err& err) {
    this->value.json.err = err.json;
    this->message = err.message;
    this->source = err.message.name;
    this->seq = err.seq;
  }

  Result::Result (const Data& data) {
    this->value.json.data = data.json;
    this->message = data.message;
    this->source = data.message.name;
    this->seq = data.seq;
  }

  Result::Result (const Message::Seq& seq, const Message& message) {
    this->message = message;
    this->source = message.name;
    this->seq = seq;
  }

  Result::Result (
    const Message::Seq& seq,
    const Message& message,
    JSON::Any json
  ) : Result(seq, message, json, runtime::Data{}) {
    // noop
  }

  Result::Result (
    const Message::Seq& seq,
    const Message& message,
    JSON::Any json,
    runtime::Data data
  ) : Result(seq, message) {
    this->value.data = data;
    if (json.type != JSON::Type::Any) {
      this->value.json.raw = json;
    }
  }

  String Result::str () const {
    return this->json().str();
  }

  runtime::Data Result::data () const {
    return this->value.data;
  }

  JSON::Any Result::json () const {
    // return result value if set explicitly
    if (this->value.json.raw.type != JSON::Type::Null) {
      return this->value.json.raw;
    }

    auto entries = JSON::Object::Entries {
      {"source", this->source},
      {"data", this->value.json.data},
      {"err", this->value.json.err}
    };

    return JSON::Object(entries);
  }

  Router::Router (Runtime& runtime)
    : runtime(runtime)
  {
    this->networkStatusObserver.onNetworkStatusChangeCallback =
      [&](const String& statusName, const String& statusMessage) {
        this->onNetworkStatusChange(statusName, statusMessage);
      };
  }

  Router::~Router () {
  }

  void Router::onNetworkStatusChange (
    const String statusName,
    const String statusMessage
  ) {
    auto json = JSON::Object::Entries {
      {"status", statusName},
      {"message", statusMessage}
    };

    this->emit(statusName, json);
  }

  bool Router::hasMappedBuffer (int index, const Message::Seq& seq) {
    Lock lock(this->mutex);
    auto key = std::to_string(index) + seq;
    return this->buffers.find(key) != this->buffers.end();
  }

  MessageBuffer Router::getMappedBuffer (int index, const Message::Seq& seq) {
    if (this->hasMappedBuffer(index, seq)) {
      Lock lock(this->mutex);
      auto key = std::to_string(index) + seq;
      return this->buffers.at(key);
    }

    return MessageBuffer {};
  }

  void Router::removeMappedBuffer (int index, const Message::Seq& seq) {
    Lock lock(this->mutex);
    if (this->hasMappedBuffer(index, seq)) {
      auto key = std::to_string(index) + seq;
      this->buffers.erase(key);
    }
  }

  void Router::setMappedBuffer (
    int index,
    const Message::Seq& seq,
    char* bytes,
    size_t size
  ) {
    Lock lock(this->mutex);
    auto key = std::to_string(index) + seq;
    this->buffers.insert_or_assign(key, MessageBuffer { bytes, size });
  }

  void Router::map (const String& name, RouteCallback callback) {
    return this->map(name, true, callback);
  }

  void Router::map (const String& name, bool async, RouteCallback callback) {
    if (callback != nullptr) {
      table.insert_or_assign(name, RouteCallbackContext { async, callback });
    }
  }

  void Router::unmap (const String& name) {
    if (table.find(name) != table.end()) {
      table.erase(name);
    }
  }

  bool Router::dispatch (DispatchFunction callback) {
    this->runtime.dispatch(callback);
    return true;
  }

  bool Router::emit (
    const String& name,
    const String& data
  ) {
    auto value = encodeURIComponent(data);
    auto script = getEmitToRenderProcessJavaScript(name, value);
    return this->evaluateJavaScript(script);
  }

  bool Router::emit (
    const String& name,
    const JSON::Any& data
  ) {
    return this->emit(name, data.str());
  }

  bool Router::evaluateJavaScript (const String& js) {
    if (this->evaluateJavaScriptFunction != nullptr) {
      this->evaluateJavaScriptFunction(js);
      return true;
    }

    return false;
  }

  bool Router::invoke (const Message& message) {
    return this->invoke(message, [this](auto result) {
      this->send(result.seq, result.json(), result.value.data);
    });
  }

  bool Router::invoke (const Message& message, ResultCallback callback) {
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

  bool Router::invoke (const String& name, char *bytes, size_t size) {
    auto message = Message { name, bytes, size };
    return this->invoke(message);
  }

  bool Router::invoke (
    const String& name,
    char *bytes,
    size_t size,
    ResultCallback callback
  ) {
    auto message = Message { name, bytes, size };
    return this->invoke(message, callback);
  }

  bool Router::send (
    const Message::Seq& seq,
    const JSON::Any& json,
    runtime::Data data
  ) {
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
}
