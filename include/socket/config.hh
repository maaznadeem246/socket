#ifndef SSC_SOCKET_CONFIG_H
#define SSC_SOCKET_CONFIG_H
#include "common.hh"
#include "json.hh"

namespace ssc::config {
  // forward
  class Config;

  extern bool isDebugEnabled ();
  extern int getServerPort ();
  extern void init (Config& config);

  class Config {
    public:
      String source;
      Map entries;
      Config () = default;
      Config (const String& source) {
        this->set(source);
      }

      bool has (const String& key) const {
        return this->entries.count(key) != 0;
      }

      const String operator [] (const String& key) const {
        if (this->entries.find(key) != this->entries.end()) {
          return this->entries.at(key);
        }

        return "";
      }

      String& operator [] (const String& key) {
        return this->entries[key];
      }

      const String get (const String& key) const {
        if (this->entries.find(key) != this->entries.end()) {
          return this->entries.at(key);
        }

        return "";
      }

      void set (const String& source) {
        this->source = source;

        auto entries = split(source, '\n');

        for (auto entry : entries) {
          auto index = entry.find_first_of(':');

          if (index >= 0 && index <= entry.size()) {
            auto key = entry.substr(0, index);
            auto value = entry.substr(index + 1);

            this->entries[trim(key)] = trim(value);
          }
        }
      }

      void set (const String& key, const String& value) {
        this->entries[key] = value;
      }

      const String str () const {
        return this->source;
      }

      auto size () const {
        return this->entries.size();
      }

      auto json () const {
        return JSON::Object(this->entries);
      }
  };
}
#endif
