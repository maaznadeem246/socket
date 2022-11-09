/**
 * @module ssc.ipc
 * @description TODO
 */
export module ssc.ipc;
export import ssc.ipc.message;
export import ssc.ipc.result;
export import ssc.ipc.router;

export namespace ssc::ipc {
  using ssc::ipc::message::Message;
  using ssc::ipc::result::Result;
  using ssc::ipc::router::Router;
  using Seq = Message::Seq;
}
