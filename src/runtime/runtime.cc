#include "runtime.hh"
#include "utils.hh"

using namespace ssc::utils;

namespace ssc::runtime {
  Headers::Headers (const Headers& headers) {
    this->entries = headers.entries;
  }

  Headers::Headers (const Vector<std::map<String, Header::Value>>& entries) {
    for (const auto& entry : entries) {
      for (const auto& pair : entry) {
        this->entries.push_back(Header { pair.first, pair.second });
      }
    }
  }

  Headers::Headers (const Headers::Entries& entries) {
    for (const auto& entry : entries) {
      this->entries.push_back(entry);
    }
  }

   Headers::Headers (const Map& map) {
     for (const auto& entry : map) {
       this->set(entry.first, entry.second);
     }
   }

   Headers::Headers (const String& source) {
     for (const auto& line: split(source, '\n')) {
       const auto pair = split(line, ':');
       this->set(pair[0], pair[1]);
     }
   }

   Headers::Headers (const char* source)
     : Headers(String(source))
   {
     // noop
   }

  void Headers::set (const String& key, const String& value) {
    for (auto& entry : this->entries) {
      if (entry.key == key) {
        entry.value.string = value;
        return;
      }
    }

    this->entries.push_back({ key, value });
  }

  bool Headers::has (const String& key) const {
    for (const auto& entry : this->entries) {
      if (entry.key == key) {
        return true;
      }
    }

    return false;
  }

  String Headers::get (const String& key) const {
    for (const auto& entry : this->entries) {
      if (entry.key == key) {
        return entry.value.str();
      }
    }

    return "";
  }

  String Headers::operator [] (const String& key) const {
    for (const auto& entry : this->entries) {
      if (entry.key == key) {
        return entry.value.str();
      }
    }

    return "";
  }

  String& Headers::operator [] (const String& key) {
    static String empty = "";

    if (!this->has(key)) {
      this->set(key, "");
    }

    for (auto& entry : this->entries) {
      if (entry.key == key) {
        return entry.value.string;
      }
    }

    return empty;
  }

  size_t Headers::size () const {
    return this->entries.size();
  }

  String Headers::str () const {
    StringStream headers;
    Map mapping;

    for (const auto& entry : this->entries) {
      mapping[entry.key] = entry.value.str();
    }

    auto count = mapping.size();
    for (const auto& entry : mapping) {
      headers << entry.first << ": " << entry.second;
      if (--count > 0) {
        headers << "\n";
      }
    }

    return headers.str();
  }

  JSON::Object Headers::json () const {
    auto json = JSON::Object {};
    for (const auto& entry : this->entries) {
      json[entry.key] = entry.value.str();
    }
    return json;
  }

  DataManager::DataManager () {
    this->entries = SharedPointer<Entries>(new Entries());
  }

  Data DataManager::get (uint64_t id) {
    Lock lock(this->mutex);

    if (this->entries->find(id) == this->entries->end()) {
      return Data{};
    }

    return this->entries->at(id);
  }

  void DataManager::put (uint64_t id, Data& data) {
    Lock lock(this->mutex);
    data.ttl = std::chrono::time_point_cast<std::chrono::milliseconds>(
      std::chrono::system_clock::now() +
      std::chrono::milliseconds(32 * 1024)
    )
      .time_since_epoch()
      .count();

    this->entries->insert_or_assign(id, data);
  }

  bool DataManager::has (uint64_t id) {
    Lock lock(this->mutex);
    return this->entries->find(id) != this->entries->end();
  }

  void DataManager::remove (uint64_t id) {
    Lock lock(this->mutex);
    if (entries->find(id) == entries->end()) return;
    auto data = this->get(id);

    this->entries->erase(id);

    if (data.body && data.bodyNeedsFree) {
      delete [] data.body;
    }
  }

  void DataManager::expire () {
    Lock lock(this->mutex);
    std::vector<uint64_t> ids;
    auto now = std::chrono::system_clock::now()
      .time_since_epoch()
      .count();

    for (auto const &tuple : *this->entries) {
      auto id = tuple.first;
      auto data = tuple.second;

      if (data.ttl < now) {
        ids.push_back(id);
      }
    }

    for (auto const id : ids) {
      remove(id);
    }
  }

  void DataManager::clear () {
    Lock lock(this->mutex);
    Vector<uint64_t> ids;

    for (auto const &tuple : *entries) {
      auto id = tuple.first;
      ids.push_back(id);
    }

    for (auto const id : ids) {
      this->remove(id);
    }
  }

  String DataManager::create (const String& seq, const JSON::Any& json, Data& data) {
    return this->create(seq, json.str(), data);
  }

  String DataManager::create (const String& seq, const String& params, Data& data) {
    Lock lock(this->mutex);

    if (data.id == 0) {
      data.id = rand64();
    }

    auto sid = std::to_string(data.id);
    auto js = createJavaScriptSource("data.js",
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
      "  const headers = `" + trim(data.headers.str()) + "`          \n"
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
      "xhr.open('GET', 'ipc://data?id=" + sid + "');                 \n"
      "xhr.send();                                                   \n"
    );

    this->put(data.id, data);
    return js;
  }

  void Runtime::start () {
    this->loops.main.start();
    this->loops.background.start();

    this->loops.main.dispatch([&] () mutable {
      //this->descriptorManager.init();
      this->timers.start();
    });
  }

  void Runtime::stop () {
    this->loops.main.stop();
    this->loops.background.stop();
    this->timers.stop();
  }

  void Runtime::dispatch (Loop::DispatchCallback cb) {
    this->loops.main.dispatch(cb);
  }

  void Runtime::wait () {
    this->loops.main.wait();
  }
}
