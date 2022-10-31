module; // global
#include "../platform.hh"

/**
 * @module runtime
 * @description TODO
 * @example
 */
export module ssc.runtime;
import ssc.javascript;
import ssc.uv;

export namespace ssc::runtime {
  using EventLoopDispatchCallback = std::function<void()>;

  class Peer;
  class Runtime {
    public:

      std::map<uint64_t, Peer*> peers;

      Mutex peersMutex;
      Mutex timersMutex;

      std::atomic<bool> didLoopInit = false;
      std::atomic<bool> didTimersInit = false;
      std::atomic<bool> didTimersStart = false;

      Runtime () {
        initEventLoop();
      }

      // loop
      uv_loop_t* getEventLoop ();
      int getEventLoopTimeout ();
      bool isLoopAlive ();
      void initEventLoop ();
      void runEventLoop ();
      void stopEventLoop ();
      void dispatchEventLoop (EventLoopDispatchCallback dispatch);
      void signalDispatchEventLoop ();
      void sleepEventLoop (int64_t ms);
      void sleepEventLoop ();
  };
}
