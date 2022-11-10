/**
 * @module ssc.ipc
 * @description TODO
 */
export module ssc.ipc;
export import ssc.ipc.message;
export import ssc.ipc.result;
export import ssc.ipc.data;

export namespace ssc::ipc {
  using message::Message;
  using result::Result;
  using data::Data;
  using Seq = message::Message::Seq;
}
