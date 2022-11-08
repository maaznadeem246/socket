module; // global
#include <new>

#include "../core/types-headers.hh"

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

export namespace ssc::JSON {
  #define SSC_INLINE_INCLUDE
  #include "../core/json.hh"
}
