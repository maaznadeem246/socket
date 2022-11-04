module; // global
#include <map>
#include <new>
#include <string>

/**
 * @module ssc.json
 * @description JSON building interfaces.
 * @example
 * import ssc.json;
 * using namespace ssc::JSON;
 *
 * auto entries = Object::Entries {
 *   {"data", Object::Entries {
 *     {"boolean", true},
 *     {"number", 123.456}
 *     {"string", "string"},
 *     {"array", Array::Entries {
 *       {"string"},
 *       {true},
 *       {987.654},
 *       {nullptr}
 *     }
 *   }}
 * };
 *
 * auto object = Object(entries);
 * auto data = object.get("data").as<Object>();
 * auto array = data.get("array").as<Array>();
 * auto string = object.str();
 */
export module ssc.json;
import ssc.types;
import ssc.string;

using namespace ssc::types;

export namespace ssc::JSON {
  // forward
  class Any;
  class Null;
  class Object;
  class Array;
  class Boolean;
  class Number;
  class String;

  using ObjectEntries = std::map<ssc::string::String, Any>;
  using ArrayEntries = Vector<Any>;

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
    public:
      Type type = t;
      virtual ssc::string::String str () const = 0;
      auto typeof () const {
        switch (this->type) {
          case Type::Any: return ssc::string::String("any");
          case Type::Array: return ssc::string::String("array");
          case Type::Boolean: return ssc::string::String("boolean");
          case Type::Number: return ssc::string::String("number");
          case Type::Null: return ssc::string::String("null");
          case Type::Object: return ssc::string::String("object");
          case Type::String: return ssc::string::String("string");
        }
      }

      auto isArray () const { return this->type == Type::Array; }
      auto isBoolean () const { return this->type == Type::Boolean; }
      auto isNumber () const { return this->type == Type::Number; }
      auto isNuLl () const { return this->type == Type::Null; }
      auto isObject () const { return this->type == Type::Object; }
      auto isString () const { return this->type == Type::String; }

    protected:
      D data;
  };

  class Any : public Value<void *, Type::Any> {
    public:
      SharedPointer<void> pointer;

      Any ();
      ~Any ();
      Any (const Any&);
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
      Any (const ssc::string::String);
      Any (const String);
      Any (const Object);
      Any (const ObjectEntries);
      Any (const Array);
      Any (const ArrayEntries);
      ssc::string::String str () const;

      template <typename T> T& as () const {
        return *reinterpret_cast<T *>(this->pointer.get());
      }
  };

  auto typeof (const Any& any) {
    return any.typeof();
  }

  class Null : Value<std::nullptr_t, Type::Null> {
    public:
      Null () = default;
      Null (std::nullptr_t) : Null() {}
      std::nullptr_t value () const {
        return nullptr;
      }

      ssc::string::String str () const {
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

      Object (const Map map) {
        for (const auto& tuple : map) {
          auto key = tuple.first;
          auto value = Any(tuple.second);
          this->data.insert_or_assign(key, value);
        }
      }

      ssc::string::String str () const {
        StringStream stream;
        auto count = this->data.size();
        stream << ssc::string::String("{");

        for (const auto& tuple : this->data) {
          auto key = tuple.first;
          auto value = tuple.second.str();

          stream << ssc::string::String("\"");
          stream << key;
          stream << ssc::string::String("\":");
          stream << value;

          if (--count > 0) {
            stream << ssc::string::String(",");
          }
        }

        stream << ssc::string::String("}");
        return stream.str();
      }

      const Object::Entries value () const {
        return this->data;
      }

      Any get (const ssc::string::String key) const {
        if (this->data.find(key) != this->data.end()) {
          return this->data.at(key);
        }

        return nullptr;
      }

      void set (const ssc::string::String key, Any value) {
        this->data[key] = value;
      }

      Any operator [] (const ssc::string::String& key) const {
        return this->data.at(key);
      }

      Any &operator [] (const ssc::string::String& key) {
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

      ssc::string::String str () const {
        ssc::string::StringStream stream;
        auto count = this->data.size();
        stream << ssc::string::String("[");

        for (const auto value : this->data) {
          stream << value.str();

          if (--count > 0) {
            stream << ssc::string::String(",");
          }
        }

        stream << ssc::string::String("]");
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

      Boolean (ssc::string::String string) {
        this->data = string.size() > 0;
      }

      bool value () const {
        return this->data;
      }

      ssc::string::String str () const {
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

      ssc::string::String str () const {
        return this->str(7);
      }

      ssc::string::String str (int precision) const {
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

  class String : Value<ssc::string::String, Type::Number> {
    public:
      String () = default;
      String (const String& data) {
        this->data = ssc::string::String(data.value());
      }

      String (const ssc::string::String data) {
        this->data = data;
      }

      String (const char data) {
        this->data = ssc::string::String(1, data);
      }

      String (const char *data) {
        this->data = ssc::string::String(data);
      }

      ssc::string::String str () const {
        return ssc::string::format(
          "\"$S\"",
          ssc::string::replace(
            ssc::string::replace(this->data, "\"", "\\\""),
              "\n",
              "\\n"
            )
        );
      }

      ssc::string::String value () const {
        return this->data;
      }

      auto size () const {
        return this->data.size();
      }
  };

  Any::Any () {
    this->pointer = nullptr;
    this->type = Type::Null;
  }

  Any::~Any () {
    this->pointer = nullptr;
    this->type = Type::Any;
  }

  Any::Any (const Any &any) {
    this->pointer = any.pointer;
    this->type = any.type;
  }

  Any::Any (const Null null) {
    this->pointer = SharedPointer<void>(new Null());
    this->type = Type::Null;
  }

  Any::Any(std::nullptr_t null) {
    this->pointer = SharedPointer<void>(new Null());
    this->type = Type::Null;
  }

  Any::Any (const char *string) {
    this->pointer = SharedPointer<void>(new String(string));
    this->type = Type::String;
  }

  Any::Any (const char string) {
    this->pointer = SharedPointer<void>(new String(string));
    this->type = Type::String;
  }

  Any::Any (const ssc::string::String string) {
    this->pointer = SharedPointer<void>(new String(string));
    this->type = Type::String;
  }

  Any::Any (const String string) {
    this->pointer = SharedPointer<void>(new String(string));
    this->type = Type::String;
  }

  Any::Any (bool boolean) {
    this->pointer = SharedPointer<void>(new Boolean(boolean));
    this->type = Type::Boolean;
  }

  Any::Any (const Boolean boolean) {
    this->pointer = SharedPointer<void>(new Boolean(boolean));
    this->type = Type::Boolean;
  }

  Any::Any (int32_t number) {
    this->pointer = SharedPointer<void>(new Number((double) number));
    this->type = Type::Number;
  }

  Any::Any (uint32_t number) {
    this->pointer = SharedPointer<void>(new Number((double) number));
    this->type = Type::Number;
  }

  Any::Any (int64_t number) {
    this->pointer = SharedPointer<void>(new Number((double) number));
    this->type = Type::Number;
  }

  Any::Any (uint64_t number) {
    this->pointer = SharedPointer<void>(new Number((double) number));
    this->type = Type::Number;
  }

  Any::Any (double number) {
    this->pointer = SharedPointer<void>(new Number((double) number));
    this->type = Type::Number;
  }

  #if defined(__APPLE__)
  Any::Any (ssize_t  number) {
    this->pointer = SharedPointer<void>(new Number((double) number));
    this->type = Type::Number;
  }
  #endif

  Any::Any (const Number number) {
    this->pointer = SharedPointer<void>(new Number(number));
    this->type = Type::Number;
  }

  Any::Any (const Object object) {
    this->pointer = SharedPointer<void>(new Object(object));
    this->type = Type::Object;
  }

  Any::Any (const Object::Entries entries) {
    this->pointer = SharedPointer<void>(new Object(entries));
    this->type = Type::Object;
  }

  Any::Any (const Array array) {
    this->pointer = SharedPointer<void>(new Array(array));
    this->type = Type::Array;
  }

  Any::Any (const Array::Entries entries) {
    this->pointer = SharedPointer<void>(new Array(entries));
    this->type = Type::Array;
  }

  ssc::string::String Any::str () const {
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
}
