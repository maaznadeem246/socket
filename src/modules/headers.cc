module;

#include "../platform.hh"

export module ssc.core:headers;
export namespace ssc {
  class Headers {
    public:
      class Value {
        public:
          String string;
          Value () = default;
          Value (String value) { this->string = value; }
          Value (const char* value) { this->string = value; }
          Value (const Value& value) { this->string = value.string; }
          Value (bool value) { this->string = value ? "true" : "false"; }
          Value (int value) { this->string = std::to_string(value); }
          Value (float value) { this->string = std::to_string(value); }
          Value (int64_t value) { this->string = std::to_string(value); }
          Value (uint64_t value) { this->string = std::to_string(value); }
          Value (double_t value) { this->string = std::to_string(value); }
#if defined(__APPLE__)
          Value (ssize_t value) { this->string = std::to_string(value); }
#endif
          String str () const { return this->string; }
      };

      class Header {
        public:
          String key;
          Value value;
          Header (const Header& header);
          Header (const String& key, const Value& value);
      };

      using Entries = Vector<Header>;
      Entries entries;
      Headers () = default;
      Headers (const Headers& headers);
      Headers (const Vector<std::map<String, Value>>& entries);
      Headers (const Entries& entries);
      size_t size () const;
      String str () const;
  };

  Headers::Header::Header (const Header& header) {
    this->key = header.key;
    this->value = header.value;
  }

  Headers::Header::Header (const String& key, const Value& value) {
    this->key = key;
    this->value = value;
  }

  Headers::Headers (const Headers& headers) {
    this->entries = headers.entries;
  }

  Headers::Headers (const Vector<std::map<String, Value>>& entries) {
    for (const auto& entry : entries) {
      for (const auto& pair : entry) {
        this->entries.push_back(Header { pair.first, pair.second });
      }
    }
  }

  Headers::Headers (const Entries& entries) {
    for (const auto& entry : entries) {
      this->entries.push_back(entry);
    }
  }

  size_t Headers::size () const {
    return this->entries.size();
  }

  String Headers::str () const {
    StringStream headers;
    auto count = this->size();
    for (const auto& entry : this->entries) {
      headers << entry.key << ": " << entry.value.str();;
      if (--count > 0) {
        headers << "\n";
      }
    }
    return headers.str();
  }
}
