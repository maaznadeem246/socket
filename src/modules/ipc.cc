/**
 * @module ssc.ipc
 * @description TODO
 */
export module ssc.ipc;
export import ssc.ipc.message;
export import ssc.ipc.result;
export import ssc.ipc.data;

export namespace ssc::ipc {
  using data::Data; // NOLINT(misc-unused-using-decls)
  using message::Message; // NOLINT(misc-unused-using-decls)
  using result::Result; // NOLINT(misc-unused-using-decls)
  using Seq = message::Message::Seq; // NOLINT(misc-unused-using-decls)
}
