module;

#include "core.hh"

export module ssc.core:headers;
export namespace ssc {
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
};
