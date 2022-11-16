module; // global
#include <socket/platform.hh>

/**
 * @module ssc.loop
 * @description TODO
 * @example
 * TODO
 */
export module ssc.timers;
import ssc.loop;
import ssc.types;
import ssc.uv;

using namespace ssc::types;
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

  class Timers {
    Vector<Timer> timers;
    AtomicBool started = false;
    Mutex mutex;
    Loop& loop;

    public:
      Timers () = delete;
      Timers (Loop& loop)
        : loop(loop)
      {
      }

      void add (Timer& timer, void* data) {
        Lock lock(this->mutex);
        timer.handle.data = data;
        uv_timer_init(this->loop.get(), &timer.handle);
        this->timers.push_back(timer);
      }

      void start () {
        Lock lock(this->mutex);

        for (auto& timer : this->timers) {
          if (timer.started) {
            uv_timer_again(&timer.handle);
          } else {
            timer.started = 0 == uv_timer_start(
              &timer.handle,
              timer.invoke,
              timer.timeout,
              timer.repeated
                ? timer.interval > 0 ? timer.interval : timer.timeout
                : 0
            );
          }
        }

        this->started = true;
      }

      void stop () {
        Lock lock(this->mutex);

        if (this->started) {
          for (auto& timer : this->timers) {
            if (timer.started) {
              uv_timer_stop(&timer.handle);
            }
          }
        }

        this->started = false;
      }
  };
}
