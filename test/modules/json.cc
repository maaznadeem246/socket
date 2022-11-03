#include <assert.h>

import ssc.log;
import ssc.json;
import ssc.string;

using namespace ssc;
using String = ssc::string::String;
using ssc::JSON::typeof;

bool test_array () {
  auto array = JSON::Array(JSON::Array::Entries {
    {true},
    {false},
    {123},
    {-123},
    {456},
    {-456},
    {123.456},
    {-123.456},
    {"hello world"},
    {'X'},
    {nullptr},
    JSON::Array::Entries {
     {0},
     {1},
     {2},
     JSON::Object::Entries {
      {"key", "value"}
     }
    }
  });

  assert(typeof(array[0]) == String("boolean"));
  assert(array[0].as<JSON::Boolean>().value() == true);

  assert(typeof(array[1]) == String("boolean"));
  assert(array[1].as<JSON::Boolean>().value() == false);

  assert(typeof(array[2]) == String("number"));
  assert(array[2].as<JSON::Number>().value() == 123);

  assert(typeof(array[3]) == String("number"));
  assert(array[3].as<JSON::Number>().value() == -123);

  assert(typeof(array[4]) == String("number"));
  assert(array[4].as<JSON::Number>().value() == 456);

  assert(typeof(array[5]) == String("number"));
  assert(array[5].as<JSON::Number>().value() == -456);

  assert(typeof(array[6]) == String("number"));
  assert(array[6].as<JSON::Number>().value() == (float) 123.456);

  assert(typeof(array[7]) == String("number"));
  assert(array[7].as<JSON::Number>().value() == (float) -123.456);

  assert(typeof(array[8]) == String("string"));
  assert(array[8].as<JSON::String>().value() == String("hello world"));

  assert(typeof(array[9]) == String("string"));
  assert(array[9].as<JSON::String>().value() == String("X"));

  assert(typeof(array[10]) == String("null"));
  assert(array[10].as<JSON::Null>().value() == nullptr);

  assert(typeof(array[11]) == String("array"));
  assert(array[11].as<JSON::Array>()[0].as<JSON::Number>().value() == 0);
  assert(array[11].as<JSON::Array>()[1].as<JSON::Number>().value() == 1);
  assert(array[11].as<JSON::Array>()[2].as<JSON::Number>().value() == 2);

  assert(
    array.str() == String(
    R"JSON([true,false,123,-123,456,-456,123.456,-123.456,"hello world","X",null,[0,1,2,{"key":"value"}]])JSON"
  ));

  auto empty_array = JSON::Array{};
  assert(empty_array.size() == 0);
  return true;
}

bool test_bool () {
  auto true_boolean = JSON::Boolean { true };
  auto int_boolean = JSON::Boolean { 123 };
  auto double_boolean = JSON::Boolean { 123.456 };

  assert(true_boolean.value() == true);
  assert(true_boolean.str() == String("true"));

  assert(int_boolean.value() == true);
  assert(int_boolean.str() == String("true"));

  assert(double_boolean.value() == true);
  assert(double_boolean.str() == String("true"));

  auto false_boolean = JSON::Boolean { false };
  auto null_boolean = JSON::Boolean { nullptr };
  auto empty_boolean = JSON::Boolean {};

  assert(false_boolean.value() == false);
  assert(false_boolean.str() == String("false"));

  assert(null_boolean.value() == false);
  assert(null_boolean.str() == String("false"));

  assert(empty_boolean.value() == false);
  assert(empty_boolean.str() == String("false"));
  return true;
}

bool test_null () {
  auto null = JSON::Null {};
  assert(null.value() == nullptr);
  assert(null.str() == String("null"));
  return true;
}

bool test_number () {
  auto zero_number = JSON::Number { 0 };
  auto one_number = JSON::Number { 1 };
  auto true_number = JSON::Number { true };
  auto false_number = JSON::Number { false };
  auto negative_float_number = JSON::Number { (float) -123.456 };
  auto positive_float_number = JSON::Number { (float) 123.456 };
  auto negative_int_number = JSON::Number { (float) -456 };
  auto positive_int_number = JSON::Number { (float) 456 };

  assert(zero_number.value() == 0);
  assert(zero_number.str() == String("0"));

  assert(one_number.value() == 1);
  assert(one_number.str() == String("1"));

  assert(true_number.value() == 1);
  assert(true_number.str() == String("1"));

  assert(false_number.value() == 0);
  assert(false_number.str() == String("0"));

  assert(negative_float_number.value() == (float) -123.456);
  assert(negative_float_number.str() == String("-123.456"));
  assert(positive_float_number.value() == (float) 123.456);
  assert(positive_float_number.str() == String("123.456"));

  assert(negative_int_number.value() == -456);
  assert(negative_int_number.str() == String("-456"));
  assert(positive_int_number.value() == 456);
  assert(positive_int_number.str() == String("456"));
  return true;
}

bool test_object () {
  return true;
}

bool test_string () {
  auto empty_string = JSON::String {};
  auto string = JSON::String { "hello world" };
  assert(empty_string.value() == String(""));
  assert(string.value() == String("hello world"));
  return true;
}

int main () {
  assert(test_array());
  assert(test_bool());
  assert(test_null());
  assert(test_number());
  assert(test_object());
  assert(test_string());
  return 0;
}
