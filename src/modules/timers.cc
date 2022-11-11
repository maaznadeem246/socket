module; // global
#include <socket/platform.hh>

/**
 * @module ssc.loop
 * @description TODO
 * @example
 * TODO
 */
export module ssc.timers;
import ssc.context;
import ssc.loop;
import ssc.uv;

using Context = ssc::context::Context;
using Loop = ssc::loop::Loop;

export namespace ssc::timers {
  // forward
  class Runtime;
  class Timers;

  struct Timer {
    uv_timer_t handle;
    bool repeated = false;
    bool started = false;
    uint64_t timeout = 0;
    uint64_t interval = 0;
    uv_timer_cb invoke;
    Timers* timers;
  };

  class Timers : Context {
    AtomicBool started;
    Mutex mutex;
    Vector<Timer> timers;
    Loop* loop;

    public:
      Timers (Runtime* runtime, Loop* loop) : Context(runtime) {
        this->loop = loop;
      }

      void add (Timer& timer, void* data) {
        Lock lock(this->mutex);
        timer.handle.data = data;
        uv_timer_init(this->loop, &timer.handle);
        this->timers.push_back(timer);
      }

      void start () {
        Lock lock(this->mutex);

        for (const auto &timer : this->timers) {
          if (timer->started) {
            uv_timer_again(&timer->handle);
          } else {
            timer->started = 0 == uv_timer_start(
              &timer->handle,
              timer->invoke,
              timer->timeout,
              !timer->repeated
                ? 0
                : timer->interval > 0
                  ? timer->interval
                  : timer->timeout
            );
          }
        }

        this->started = true;
      }

      void stop () {
        Lock lock(this->mutex);

        if (this->started) {
          for (const auto& timer : timersToStop) {
            if (timer->started) {
              uv_timer_stop(&timer->handle);
            }
          }
        }

        this->started = false;
      }
  };
}
