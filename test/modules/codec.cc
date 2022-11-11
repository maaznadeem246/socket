#include <assert.h>

#include <socket/runtime.hh>
import ssc.string;
import ssc.codec;


int main () {
  using namespace ssc::codec;
  using namespace ssc::string;

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
