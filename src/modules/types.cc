module;
#include "../core/platform.hh"

/**
 * @module ssc.types
 * @description Exports various types commonly used in ssc modules.
 * @example
 * imoprt ssc.types;
 * namespace ssc {
 *   using namespace ssc::types;
 *   String string;
 *   AtomicBool boolean;
 *   Map kv;
 *   Mutex mutex;
 * }
 */
export module ssc.types;
export namespace ssc::types {
  #define SSC_INLINE_INCLUDE
  #include "../core/types.hh"
}
