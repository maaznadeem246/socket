#ifndef SSC_CORE_JSON_HH
#define SSC_CORE_JSON_HH

#if !defined(SSC_INLINE_INCLUDE)
#include "platform.hh"
#endif

#if !defined(SSC_INLINE_INCLUDE)
namespace ssc::JSON {
#endif

  // forward
  class Any;
  class Null;
  class Object;
  class Array;
  class Boolean;
  class Number;
  class String;

  using ObjectEntries = std::map<std::string, Any>;
  using ArrayEntries = std::vector<Any>;

  enum class Type {
    Any,
    Null,
    Object,
    Array,
    Boolean,
    Number,
    String
  };

  template <typename D, Type t> class Value {
    protected:
      D data;

    public:
      Type type = t;
      virtual std::string str () const = 0;
      auto typeof () const {
        switch (this->type) {
          case Type::Any: return std::string("any");
          case Type::Array: return std::string("array");
          case Type::Boolean: return std::string("boolean");
          case Type::Number: return std::string("number");
          case Type::Null: return std::string("null");
          case Type::Object: return std::string("object");
          case Type::String: return std::string("string");
        }
      }

      auto isArray () const { return this->type == Type::Array; }
      auto isBoolean () const { return this->type == Type::Boolean; }
      auto isNumber () const { return this->type == Type::Number; }
      auto isNuLl () const { return this->type == Type::Null; }
      auto isObject () const { return this->type == Type::Object; }
      auto isString () const { return this->type == Type::String; }
  };

  class Any : public Value<void *, Type::Any> {
    public:
      std::shared_ptr<void> pointer;

      Any () {
        this->pointer = nullptr;
        this->type = Type::Null;
      }

      ~Any () {
        this->pointer = nullptr;
        this->type = Type::Any;
      }

      Any (const Any &any) {
        this->pointer = any.pointer;
        this->type = any.type;
      }

      Any (std::nullptr_t);
      Any (const Null);
      Any (bool);
      Any (const Boolean);
      Any (int64_t);
      Any (uint64_t);
      Any (uint32_t);
      Any (int32_t);
      Any (double);
      #if defined(__APPLE__)
      Any (ssize_t);
      #endif
      Any (const Number);
      Any (const char);
      Any (const char *);
      Any (const std::string);
      Any (const String);
      Any (const Object);
      Any (const ObjectEntries);
      Any (const Array);
      Any (const ArrayEntries);
      std::string str () const;

      template <typename T> T& as () const {
        return *reinterpret_cast<T *>(this->pointer.get());
      }
  };

  inline const auto typeof (const Any& any) {
    return any.typeof();
  }

  class Null : Value<std::nullptr_t, Type::Null> {
    public:
      Null () = default;
      Null (std::nullptr_t) : Null() {}
      std::nullptr_t value () const {
        return nullptr;
      }

      std::string str () const {
        return "null";
      }
  };

  class Object : Value<ObjectEntries, Type::Object> {
    public:
      using Entries = ObjectEntries;
      Object () = default;
      Object (const Object::Entries entries) {
        for (const auto& tuple : entries) {
          auto key = tuple.first;
          auto value = tuple.second;
          this->data.insert_or_assign(key, value);
        }
      }

      Object (const Object& object) {
        this->data = object.value();
      }

      Object (const std::map<std::string, std::string> map) {
        for (const auto& tuple : map) {
          auto key = tuple.first;
          auto value = Any(tuple.second);
          this->data.insert_or_assign(key, value);
        }
      }

      std::string str () const {
        std::stringstream stream;
        auto count = this->data.size();
        stream << std::string("{");

        for (const auto& tuple : this->data) {
          auto key = tuple.first;
          auto value = tuple.second.str();

          stream << std::string("\"");
          stream << key;
          stream << std::string("\":");
          stream << value;

          if (--count > 0) {
            stream << std::string(",");
          }
        }

        stream << std::string("}");
        return stream.str();
      }

      const Object::Entries value () const {
        return this->data;
      }

      Any get (const std::string key) const {
        if (this->data.find(key) != this->data.end()) {
          return this->data.at(key);
        }

        return nullptr;
      }

      void set (const std::string key, Any value) {
        this->data[key] = value;
      }

      Any operator [] (const std::string& key) const {
        return this->data.at(key);
      }

      Any &operator [] (const std::string& key) {
        return this->data[key];
      }

      auto size () const {
        return this->data.size();
      }
  };

  class Array : Value<ArrayEntries, Type::Array> {
    public:
      using Entries = ArrayEntries;
      Array () = default;
      Array (const Array& array) {
        this->data = array.value();
      }

      Array (const Array::Entries entries) {
        for (const auto& value : entries) {
          this->data.push_back(value);
        }
      }

      std::string str () const {
        std::stringstream stream;
        auto count = this->data.size();
        stream << std::string("[");

        for (const auto value : this->data) {
          stream << value.str();

          if (--count > 0) {
            stream << std::string(",");
          }
        }

        stream << std::string("]");
        return stream.str();
      }

      Array::Entries value () const {
        return this->data;
      }

      Any get (const unsigned int index) const {
        if (index < this->data.size()) {
          return this->data.at(index);
        }

        return nullptr;
      }

      void set (const unsigned int index, Any value) {
        if (index >= this->data.size()) {
          this->data.resize(index + 1);
        }

        this->data[index] = value;
      }

      Any operator [] (const unsigned int index) const {
        if (index >= this->data.size()) {
          return nullptr;
        }

        return this->data.at(index);
      }

      Any &operator [] (const unsigned int index) {
        if (index >= this->data.size()) {
          this->data.resize(index + 1);
        }

        return this->data.at(index);
      }

      auto size () const {
        return this->data.size();
      }
  };

  class Boolean : Value<bool, Type::Boolean> {
    public:
      Boolean () = default;
      Boolean (const Boolean& boolean) {
        this->data = boolean.value();
      }

      Boolean (bool boolean) {
        this->data = boolean;
      }

      Boolean (int data) {
        this->data = data != 0;
      }

      Boolean (int64_t data) {
        this->data = data != 0;
      }

      Boolean (double data) {
        this->data = data != 0;
      }

      Boolean (void *data) {
        this->data = data != nullptr;
      }

      Boolean (std::string string) {
        this->data = string.size() > 0;
      }

      bool value () const {
        return this->data;
      }

      std::string str () const {
        return this->data ? "true" : "false";
      }
  };

  class Number : Value<double, Type::Number> {
    public:
      Number () = default;
      Number (const Number& number) {
        this->data = number.value();
      }

      Number (double number) {
        this->data = number;
      }

      Number (char number) {
        this->data = (double) number;
      }

      Number (int number) {
        this->data = (double) number;
      }

      Number (int64_t number) {
        this->data = (double) number;
      }

      Number (bool number) {
        this->data = (double) number;
      }

      float value () const {
        return this->data;
      }

      std::string str () const {
        return this->str(7);
      }

      std::string str (int precision) const {
        if (this->data == 0) {
          return "0";
        }

        auto value = this->data;
        auto output = std::to_string(value);

        // trim trailing zeros
        auto  i = output.size() - 1;
        for (; i >= 0; --i) {
          auto ch = output[i];
          if (ch != '0' && ch != '.') {
            break;
          }
        }

        return output.substr(0, i + 1);
      }
  };

  class String : Value<std::string, Type::Number> {
    static inline auto escape (
      const std::string &source,
      const std::string &pattern,
      const std::string &value
    ) {
      return std::regex_replace(source, std::regex(pattern), value);
    }

    public:
      String () = default;
      String (const String& data) {
        this->data = std::string(data.value());
      }

      String (const std::string data) {
        this->data = data;
      }

      String (const char data) {
        this->data = std::string(1, data);
      }

      String (const char *data) {
        this->data = std::string(data);
      }

      std::string str () const {
        auto escaped = escape(this->data, "\"", "\\\"");
        return "\"" + escape(escaped, "\n", "\\n") + "\"";
      }

      std::string value () const {
        return this->data;
      }

      auto size () const {
        return this->data.size();
      }
  };


  inline Any::Any (const Null null) {
    this->pointer = std::shared_ptr<void>(new Null());
    this->type = Type::Null;
  }

  inline Any::Any (std::nullptr_t) {
    this->pointer = std::shared_ptr<void>(new Null());
    this->type = Type::Null;
  }

  inline Any::Any (const char *string) {
    this->pointer = std::shared_ptr<void>(new String(string));
    this->type = Type::String;
  }

  inline Any::Any (const char string) {
    this->pointer = std::shared_ptr<void>(new String(string));
    this->type = Type::String;
  }

  inline Any::Any (const std::string string) {
    this->pointer = std::shared_ptr<void>(new String(string));
    this->type = Type::String;
  }

  inline Any::Any (const String string) {
    this->pointer = std::shared_ptr<void>(new String(string));
    this->type = Type::String;
  }

  inline Any::Any (bool boolean) {
    this->pointer = std::shared_ptr<void>(new Boolean(boolean));
    this->type = Type::Boolean;
  }

  inline Any::Any (const Boolean boolean) {
    this->pointer = std::shared_ptr<void>(new Boolean(boolean));
    this->type = Type::Boolean;
  }

  inline Any::Any (int32_t number) {
    this->pointer = std::shared_ptr<void>(new Number((double) number));
    this->type = Type::Number;
  }

  inline Any::Any (uint32_t number) {
    this->pointer = std::shared_ptr<void>(new Number((double) number));
    this->type = Type::Number;
  }

  inline Any::Any (int64_t number) {
    this->pointer = std::shared_ptr<void>(new Number((double) number));
    this->type = Type::Number;
  }

  inline Any::Any (uint64_t number) {
    this->pointer = std::shared_ptr<void>(new Number((double) number));
    this->type = Type::Number;
  }

  inline Any::Any (double number) {
    this->pointer = std::shared_ptr<void>(new Number((double) number));
    this->type = Type::Number;
  }

  #if defined(__APPLE__)
  inline Any::Any (ssize_t  number) {
    this->pointer = std::shared_ptr<void>(new Number((double) number));
    this->type = Type::Number;
  }
  #endif

  inline Any::Any (const Number number) {
    this->pointer = std::shared_ptr<void>(new Number(number));
    this->type = Type::Number;
  }

  inline Any::Any (const Object object) {
    this->pointer = std::shared_ptr<void>(new Object(object));
    this->type = Type::Object;
  }

  inline Any::Any (const Object::Entries entries) {
    this->pointer = std::shared_ptr<void>(new Object(entries));
    this->type = Type::Object;
  }

  inline Any::Any (const Array array) {
    this->pointer = std::shared_ptr<void>(new Array(array));
    this->type = Type::Array;
  }

  inline Any::Any (const Array::Entries entries) {
    this->pointer = std::shared_ptr<void>(new Array(entries));
    this->type = Type::Array;
  }

  inline std::string Any::str () const {
    auto ptr = this->pointer.get();

    switch (this->type) {
      case Type::Any: return "";
      case Type::Null: return Null().str();
      case Type::Object: return reinterpret_cast<Object *>(ptr)->str();
      case Type::Array: return reinterpret_cast<Array *>(ptr)->str();
      case Type::Boolean: return reinterpret_cast<Boolean *>(ptr)->str();
      case Type::Number: return reinterpret_cast<Number *>(ptr)->str();
      case Type::String: return reinterpret_cast<String *>(ptr)->str();
    }

    return "";
  }

#if !defined(SSC_INLINE_INCLUDE)
}
#endif

#endif
