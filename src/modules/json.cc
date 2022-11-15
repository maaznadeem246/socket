module; // global
#include "../core/json.hh"

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
  using Type = ssc::core::JSON::Type;
  using Any = ssc::core::JSON::Any;
  using Null = ssc::core::JSON::Null;
  using Object = ssc::core::JSON::Object;
  using Array = ssc::core::JSON::Array;
  using Boolean = ssc::core::JSON::Boolean;
  using Number = ssc::core::JSON::Number;
  using String = ssc::core::JSON::String;

  const auto& typeof = ssc::core::JSON::typeof;
}
