import ssc.json;
#include <stdio.h>
#include <assert.h>
#include "../../src/common.hh"

using namespace ssc;

bool test_array () {
  return true;
}

bool test_bool () {
  auto true_boolean = JSON::Boolean { true };
  auto false_boolean = JSON::Boolean { true };
  assert(true_boolean.value() == true);
  assert(true_boolean.str() == String("true"));
  return true;
}

bool test_null () {
  return true;
}

bool test_number () {
  return true;
}

bool test_object () {
  return true;
}

bool test_string () {
  return true;
}

int main () {
  assert(test_bool());
  assert(test_string());
  assert(test_number());
  printf("ok\n");
  return 0;
}
