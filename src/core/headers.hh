#ifndef SSC_CORE_HEADERS_HH
#define SSC_CORE_HEADERS_HH

#include <socket/platform.hh>
#include "string.hh"
#include "types.hh"
#include "json.hh"

namespace ssc::core::headers {
  using namespace ssc::core::string;
  using namespace ssc::core::types;

  /**
    * A container for a header value supporting varying types in the
    * constructor. All values are converted to a string.
    */
  class Value {
    public:
      String string;
      Value () = default;
      Value (String value) {
        this->string = value;
      }

      Value (const char* value) {
        this->string = value;
      }

      Value (const Value& value) {
        this->string = value.string;
      }

      Value (bool value) {
        this->string = value ? "true" : "false";
      }

      Value (int value) {
        this->string = std::to_string(value);
      }

      Value (float value) {
        this->string = std::to_string(value);
      }

      Value (int64_t value) {
        this->string = std::to_string(value);
      }

      Value (uint64_t value) {
        this->string = std::to_string(value);
      }

      Value (double_t value) {
        this->string = std::to_string(value);
      }

      #if defined(__APPLE__)
      Value (ssize_t value) {
        this->string = std::to_string(value);
      }
      #endif

      String str () const {
        return this->string;
      }
  };

  /**
    * A key-value mapping for a single header entry.
    */
  class Header {
    public:
      String key;
      Value value;
      Header () = default;
      Header (const Header& header) {
        this->key = header.key;
        this->value = header.value;
      }

      Header (const String& key, const Value& value) {
        this->key = key;
        this->value = value;
      }
  };

  /**
   * A container for new line delimited headers.
   */
  class Headers {
    public:
      using Entries = Vector<Header>;
      Entries entries;
      Headers () = default;
      Headers (const Headers& headers) {
        this->entries = headers.entries;
      }

      Headers (const Vector<std::map<String, Value>>& entries) {
        for (const auto& entry : entries) {
          for (const auto& pair : entry) {
            this->entries.push_back(Header { pair.first, pair.second });
          }
        }
      }

      Headers (const Entries& entries) {
        for (const auto& entry : entries) {
          this->entries.push_back(entry);
        }
      }

      Headers (const Map& map) {
        for (const auto& entry : map) {
          this->set(entry.first, entry.second);
        }
      }

      Headers (const String& source) {
        for (const auto& line: split(source, '\n')) {
          const auto pair = split(line, ':');
          this->set(pair[0], pair[1]);
        }
      }

      Headers (const char* source) : Headers(String(source)) {
        // noop
      }

      void set (const String& key, const String& value) {
        for (auto& entry : this->entries) {
          if (entry.key == key) {
            entry.value.string = value;
            return;
          }
        }

        this->entries.push_back({ key, value });
      }

      bool has (const String& key) const {
        for (const auto& entry : this->entries) {
          if (entry.key == key) {
            return true;
          }
        }

        return false;
      }

      String get (const String& key) const {
        for (const auto& entry : this->entries) {
          if (entry.key == key) {
            return entry.value.str();
          }
        }

        return "";
      }

      String operator [] (const String& key) const {
        for (const auto& entry : this->entries) {
          if (entry.key == key) {
            return entry.value.str();
          }
        }

        return "";
      }

      String &operator [] (const String& key) {
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

      size_t size () const {
        return this->entries.size();
      }

      String str () const {
        Map mapping;
        for (const auto& entry : this->entries) {
          mapping[entry.key] = entry.value.str();
        }

        StringStream headers;
        auto count = mapping.size();
        for (const auto& entry : mapping) {
          headers << entry.first << ": " << entry.second;
          if (--count > 0) {
            headers << "\n";
          }
        }
        return headers.str();
      }

      JSON::Object json () const {
        auto json = JSON::Object {};
        for (const auto& entry : this->entries) {
          json[entry.key] = entry.value.str();
        }
        return json;
      }
  };
}
#endif
