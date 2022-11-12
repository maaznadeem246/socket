module;
#include "../core/types.hh"

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
  using ssc::core::types::Any;
  using ssc::core::types::AtomicBool;
  using ssc::core::types::BinarySemaphore;
  using ssc::core::types::ExitCallback;
  using ssc::core::types::Function;
  using ssc::core::types::Map;
  using ssc::core::types::Lock;
  using ssc::core::types::MessageCallback;
  using ssc::core::types::Mutex;
  using ssc::core::types::Semaphore;
  using ssc::core::types::SharedPointer;
  using ssc::core::types::UniquePointer;
  using ssc::core::types::String;
  using ssc::core::types::StringStream;
  using ssc::core::types::Thread;
  using ssc::core::types::Queue;
  using ssc::core::types::Vector;
  using ssc::core::types::WString;
  using ssc::core::types::WStringStream;

  // FIXME
  using Post = ssc::core::types::Post;
  using ssc::core::types::Posts;
}
