#ifndef SSC_CORE_DATA_HH
#define SSC_CORE_DATA_HH

#include <socket/platform.hh>
#include "headers.hh"
#include "javascript.hh"
#include "json.hh"
#include "string.hh"
#include "types.hh"
#include "utils.hh"

namespace ssc::core::data {
  using headers::Headers;
  using string::String;
  using string::trim;
  using types::Lock;
  using types::Mutex;
  using types::SharedPointer;
  using types::Vector;
  using utils::rand64;

  struct CoreData {
    uint64_t id = 0;
    uint64_t ttl = 0;
    char *body = nullptr;
    size_t length = 0;
    Headers headers = "";
    bool bodyNeedsFree = false;
  };

  class CoreDataManager {
    public:
      using Entries = std::map<uint64_t, CoreData>;
      SharedPointer<Entries> entries;
      Mutex mutex;

      CoreDataManager () {
        this->entries = SharedPointer<Entries>(new Entries());
      }

      CoreDataManager (const CoreDataManager& dataManager) = delete;

      CoreData get (uint64_t id) {
        Lock lock(this->mutex);

        if (this->entries->find(id) == this->entries->end()) {
          return CoreData{};
        }

        return this->entries->at(id);
      }

      void put (uint64_t id, CoreData& data) {
        Lock lock(this->mutex);
        data.ttl = std::chrono::time_point_cast<std::chrono::milliseconds>(
          std::chrono::system_clock::now() +
          std::chrono::milliseconds(32 * 1024)
        )
          .time_since_epoch()
          .count();

        this->entries->insert_or_assign(id, data);
      }

      bool has (uint64_t id) {
        Lock lock(this->mutex);
        return this->entries->find(id) != this->entries->end();
      }

      void remove (uint64_t id) {
        Lock lock(this->mutex);
        if (entries->find(id) == entries->end()) return;
        auto data = this->get(id);

        this->entries->erase(id);

        if (data.body && data.bodyNeedsFree) {
          delete [] data.body;
        }
      }

      void expire () {
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

      void clear () {
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

      String create (const String& seq, const JSON::Any& json, CoreData& data) {
        return this->create(seq, json.str(), data);
      }

      String create (const String& seq, const String& params, CoreData& data) {
        Lock lock(this->mutex);

        if (data.id == 0) {
          data.id = rand64();
        }

        auto sid = std::to_string(data.id);
        auto js = javascript::createJavaScript("data.js",
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
          "  const headers = `" + trim(data.headers.str()) + "`                \n"
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
  };
}
#endif
