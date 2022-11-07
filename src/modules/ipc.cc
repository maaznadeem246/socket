/**
 * @module ssc.ipc
 * @description TODO
 */
export module ssc.ipc;
import ssc.ipc.message;
import ssc.ipc.result;
import ssc.ipc.router;

export namespace ssc::ipc {
  using Message = ssc::ipc::message::Message;
  using Result = ssc::ipc::result::Result;
  using Router = ssc::ipc::router::Router;
  using Seq = Message::Seq;
}
