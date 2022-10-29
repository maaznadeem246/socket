/**
 * Globals
 */
module;

#include "../common.hh"

/**
 * Module exports.
 */
export module json;
export namespace ssc::JSON {
  enum class Type {
    Any,
    Null,
    Object,
    Array,
    Boolean,
    Number,
    String
  };

  class Any;
  class Null;
  class Object;
  class Array;
  class Boolean;
  class Number;
  class String;

  using ObjectEntries = std::map<ssc::String, Any>;
  using ArrayEntries = ssc::Vector<Any>;

  template <typename D, Type t> class Value {
    public:
      Type type = t;
      virtual ssc::String str () const = 0;

    protected:
      D data;
  };

  class Any : public Value<void *, Type::Any> {
    public:
      std::shared_ptr<void> pointer;

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
      Any (float);
      Any (double);
#if defined(__APPLE__)
      Any (ssize_t);
#endif
      Any (const Number);
      Any (const char *);
      Any (const ssc::String);
      Any (const String);
      Any (const Object);
      Any (const ObjectEntries);
      Any (const Array);
      Any (const ArrayEntries);
      ssc::String str () const;
  };

  class Null : Value<std::nullptr_t, Type::Null> {
    public:
      Null () = default;
      std::nullptr_t value () const { return nullptr; }
      ssc::String str () const { return "null"; }
  };

  class Object : Value<ObjectEntries, Type::Object> {
    public:
      using Entries = ObjectEntries;
      Object () = default;
      Object (const Object::Entries entries);
      Object (const Object& object) { this->data = object.value(); }
      Object (const ssc::Map map);
      ssc::String str () const;
      const Object::Entries value () const { return this->data; }
  };

  class Array : Value<ArrayEntries, Type::Array> {
    public:
      using Entries = ArrayEntries;
      Array () = default;
      Array (const Array& array) { this->data = array.value(); }
      Array (const Array::Entries entries);
      ssc::String str () const;
      Array::Entries value () const { return this->data; }
  };

  class Boolean : Value<bool, Type::Boolean> {
    public:
      Boolean () = default;
      Boolean (const Boolean& boolean) { this->data = boolean.value(); }
      Boolean (bool boolean) { this->data = boolean; }
      Boolean (int64_t data) { this->data = data != 0; }
      Boolean (double data) { this->data = data != 0; }
      Boolean (void *data) { this->data = data != nullptr; }
      bool value () const { return this->data; }
      ssc::String str () const { return this->data ? "true" : "false"; }
  };

  class Number : Value<float, Type::Number> {
    public:
      Number () = default;
      Number (const Number& number) { this->data = number.value(); }
      Number (float number) { this->data = number; }
      Number (int64_t number) { this->data = (float) number; }
      float value () const { return this->data; }
      ssc::String str () const {
        auto remainer = this->data - (int32_t) this->data;
        if (remainer > 0) {
          return ssc::format("$S", std::to_string(this->data));
        } else {
          return ssc::format("$S", std::to_string((int32_t) this->data));
        }
      }
  };

  class String : Value<ssc::String, Type::Number> {
    public:
      String () = default;
      String (const String& data) { this->data = ssc::String(data.str()); }
      String (const ssc::String data) { this->data = data; }
      String (const char *data) { this->data = ssc::String(data); }
      ssc::String str ()  const { return ssc::format("\"$S\"", this->data); }
      ssc::String value () const { return this->data; }
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
    this->pointer = std::shared_ptr<void>(new Null());
    this->type = Type::Null;
  }

  Any::Any(std::nullptr_t null) {
    this->pointer = std::shared_ptr<void>(new Null());
    this->type = Type::Null;
  }

  Any::Any (const char *string) {
    this->pointer = std::shared_ptr<void>(new String(string));
    this->type = Type::String;
  }

  Any::Any (const ssc::String string) {
    this->pointer = std::shared_ptr<void>(new String(string));
    this->type = Type::String;
  }

  Any::Any (const String string) {
    this->pointer = std::shared_ptr<void>(new String(string));
    this->type = Type::String;
  }

  Any::Any (bool boolean) {
    this->pointer = std::shared_ptr<void>(new Boolean(boolean));
    this->type = Type::Boolean;
  }

  Any::Any (const Boolean boolean) {
    this->pointer = std::shared_ptr<void>(new Boolean(boolean));
    this->type = Type::Boolean;
  }

  Any::Any (int32_t number) {
    this->pointer = std::shared_ptr<void>(new Number((float) number));
    this->type = Type::Number;
  }

  Any::Any (uint32_t number) {
    this->pointer = std::shared_ptr<void>(new Number((float) number));
    this->type = Type::Number;
  }

  Any::Any (int64_t number) {
    this->pointer = std::shared_ptr<void>(new Number((float) number));
    this->type = Type::Number;
  }

  Any::Any (uint64_t number) {
    this->pointer = std::shared_ptr<void>(new Number((float) number));
    this->type = Type::Number;
  }

  Any::Any (double number) {
    this->pointer = std::shared_ptr<void>(new Number((float) number));
    this->type = Type::Number;
  }

#if defined(__APPLE__)
  Any::Any (ssize_t  number) {
    this->pointer = std::shared_ptr<void>(new Number((float) number));
    this->type = Type::Number;
  }
#endif

  Any::Any (float number) {
    this->pointer = std::shared_ptr<void>(new Number(number));
    this->type = Type::Number;
  }

  Any::Any (const Number number) {
    this->pointer = std::shared_ptr<void>(new Number(number));
    this->type = Type::Number;
  }

  Any::Any (const Object object) {
    this->pointer = std::shared_ptr<void>(new Object(object));
    this->type = Type::Object;
  }

  Any::Any (const Object::Entries entries) {
    this->pointer = std::shared_ptr<void>(new Object(entries));
    this->type = Type::Object;
  }

  Any::Any (const Array array) {
    this->pointer = std::shared_ptr<void>(new Array(array));
    this->type = Type::Array;
  }

  Any::Any (const Array::Entries entries) {
    this->pointer = std::shared_ptr<void>(new Array(entries));
    this->type = Type::Array;
  }

  ssc::String Any::str () const {
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

  Object::Object (const Object::Entries entries) {
    for (const auto& tuple : entries) {
      auto key = tuple.first;
      auto value = tuple.second;
      this->data.insert_or_assign(key, value);
    }
  }

  Object::Object (const ssc::Map map) {
    for (const auto& tuple : map) {
      auto key = tuple.first;
      auto value = Any(tuple.second);
      this->data.insert_or_assign(key, value);
    }
  }

  ssc::String Object::str () const {
    StringStream stream;
    auto count = this->data.size();
    stream << "{";

    for (const auto& tuple : this->data) {
      auto key = tuple.first;
      auto value = tuple.second.str();

      stream << "\"" << key << "\":";
      stream << value;

      if (--count > 0) {
        stream << ",";
      }
    }

    stream << "}";
    return stream.str();
  }

  Array::Array (const Array::Entries entries) {
    for (const auto& value : entries) {
      this->data.push_back(value);
    }
  }

  ssc::String Array::str () const {
    ssc::StringStream stream;
    auto count = this->data.size();
    stream << "[";

    for (const auto& value : this->data) {
      stream << value.str();

      if (--count > 0) {
        stream << ",";
      }
    }

    stream << "]";
    return stream.str();
  }
}
