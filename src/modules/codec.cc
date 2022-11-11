module;
#include "../core/codec.hh"

/**
 * @module ssc.codec
 * @description Various utiltity encode/decode functions
 * @example
 * TODO
 */
export module ssc.codec;
export namespace ssc::codec {
  using ssc::core::codec::decodeUTF8; // NOLINT(misc-unused-using-decls)
  using ssc::core::codec::decodeURIComponent; // NOLINT(misc-unused-using-decls)
  using ssc::core::codec::encodeURIComponent; // NOLINT(misc-unused-using-decls)
}
