#include "runtime.hh"

namespace ssc::runtime {
  void Timers::add (Timer& timer, void* data) {
    timer.handle.data = data;
    uv_timer_init(this->loop.get(), &timer.handle);
    this->timers.push_back(timer);
  }

  void Timers::start () {
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

  void Timers::stop () {
    if (this->started) {
      for (auto& timer : this->timers) {
        if (timer.started) {
          uv_timer_stop(&timer.handle);
        }
      }
    }

    this->started = false;
  }
}
