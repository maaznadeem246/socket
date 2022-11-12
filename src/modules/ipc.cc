/**
 * @module ssc.ipc
 * @description TODO
 */
export module ssc.ipc;
export import ssc.ipc.message;
export import ssc.ipc.result;

export namespace ssc::ipc {
  using message::Message; // NOLINT(misc-unused-using-decls)
  using result::Result; // NOLINT(misc-unused-using-decls)
  using Seq = message::Message::Seq; // NOLINT(misc-unused-using-decls)
}
