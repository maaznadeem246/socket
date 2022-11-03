#include <assert.h>

import ssc.codec;
import ssc.string;
import ssc.log;

using String = ssc::string::String;

using namespace ssc;
using namespace ssc::codec;

int main () {
  assert(
    decodeURIComponent("betty%20aime%20le%20fromage%20fran%C3%A7ais") ==
    String("betty aime le fromage français")
  );

  assert(
    encodeURIComponent("betty aime le fromage français") ==
    String("betty%20aime%20le%20fromage%20fran%C3%A7ais")
  );

  assert(decodeURIComponent("") == String(""));
  assert(encodeURIComponent("") == String(""));

  return 0;
}
