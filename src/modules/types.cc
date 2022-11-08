module;

#include "../core/platform-headers.hh"

/**
 * @module ssc.types
 * @description Exports various types commonly used in ssc modules.
 */
export module ssc.types;
export namespace ssc::types {
  #define SSC_INLINE_INCLUDE
  #include "../core/types.hh"
}
