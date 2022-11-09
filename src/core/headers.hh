#ifndef SSC_CORE_HEADERS_HH
#define SSC_CORE_HEADERS_HH

#if !defined(SSC_INLINE_INCLUDE)
#include "platform.hh"
#include "string.hh"
#include "types.hh"
#endif

#if !defined(SSC_INLINE_INCLUDE)
namespace ssc::headers {
  using namespace ssc::string;
  using namespace ssc::types;
#endif

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

      size_t size () const {
        return this->entries.size();
      }

      String str () const {
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

#if !defined(SSC_INLINE_INCLUDE)
}
#endif

#endif
