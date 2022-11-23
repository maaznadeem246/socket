#include <socket/platform.hh>
#include <functional>
#include <thread>

#include "runtime.hh"

namespace ssc::runtime {
  // forward
  class Loop;
  void pollEventLoop (Loop* loop);

  /**
   * QoS for the dispatch queue used on Apple's platforms.
   * @see https://developer.apple.com/documentation/dispatch/dispatch_queue_attr_t
   */
  #if defined(__APPLE__)
  dispatch_queue_attr_t DISPATCH_QUEUE_QOS = dispatch_queue_attr_make_with_qos_class(
    DISPATCH_QUEUE_SERIAL,
    QOS_CLASS_DEFAULT,
    -1
  );

  auto constexpr DISPATCH_QUEUE_LABEL = "co.socketsupply.queue.loop";
  #endif

  Loop::Loop () {
    this->semaphores.poll.release();
    this->semaphores.signal.release();
    #if defined(__APPLE__)
      this->dispatchQueue = dispatch_queue_create(
        DISPATCH_QUEUE_LABEL,
        DISPATCH_QUEUE_QOS
      );
    #endif
  }

  Loop::~Loop () {
    this->stop();
  }

  uv_loop_t* Loop::get () {
    return &this->loop;
  }

  bool Loop::isInitialized () const {
    return this->initialized;
  }

  bool Loop::isAlive () {
    return this->isInitialized() && uv_loop_alive(&this->loop);
  }

  bool Loop::isRunning () {
    return this->isInitialized() && this->running;
  }

  int Loop::timeout () {
    uv_update_time(&this->loop);
    return uv_backend_timeout(&this->loop);
  }

  int Loop::run (uv_run_mode mode) {
    return uv_run(&this->loop, mode);
  }

  int Loop::run () {
    return this->run(UV_RUN_DEFAULT);
  }

  Loop& Loop::sleep (int64_t ms) {
    if (ms > 0) {
      auto timeout = this->timeout();
      ms = timeout > ms ? timeout : ms;
      std::this_thread::sleep_for(std::chrono::milliseconds(ms));
    }

    return *this;
  }

  Loop& Loop::sleep () {
    return this->sleep(this->timeout());
  }

  Loop& Loop::wait () {
    this->semaphores.poll.acquire();
    this->semaphores.poll.release();
    return *this;
  }

  Loop& Loop::init () {
    if (this->initialized) {
      return *this;
    }

    this->semaphores.signal.acquire();
    this->semaphores.poll.acquire();

    /**
      * Below we configure a table of `GSourceFuncs` used for
      * polling and running the uv event loop along side the one
      * used by GTK/glib) on Linux.
      * @see https://api.gtkd.org/glib.c.types.GSourceFuncs.html
      */
    #if defined(__linux__) && !defined(__ANDROID__)
      auto source = g_source_new(&loopSourceFunctions, sizeof(LoopSource));
      auto loopSource = (LoopSource *) source;
      loopSource->loop = this;
      loopSource->tag = g_source_add_unix_fd(
        source,
        uv_backend_fd(&eventLoop),
        (GIOCondition) (G_IO_IN | G_IO_OUT | G_IO_ERR)
      );

      g_source_attach(source, nullptr);
    #endif

    this->initialized = true;

    Lock lock(this->mutex);
    uv_loop_init(&this->loop);
    this->async.data = this;
    uv_async_init(&this->loop, &this->async, [](uv_async_t *handle) {
      auto loop = reinterpret_cast<Loop*>(handle->data);

      while (true) {
        if (!loop->isRunning()) {
          break;
        }

        Lock lock(loop->mutex);
        if (loop->queue.size() == 0) {
          break;
        }

        auto dispatch = loop->queue.front();
        loop->queue.pop();
        if (dispatch != nullptr) {
          dispatch();
        }
      }
    });

    return *this;
  }

  Loop& Loop::stop () {
    this->running = false;
    uv_stop(&this->loop);

    Lock lock(this->mutex);
    if (this->thread != nullptr) {
      if (this->thread->joinable()) {
        this->thread->join();
      }

      delete this->thread;
      this->thread = nullptr;
    }

    return *this;
  }

  Loop& Loop::start () {
    if (this->isRunning()) {
      return *this;
    }

    this->running = true;
    this->init();

    #if defined(__APPLE__)
      dispatch_async(this->dispatchQueue, ^{
        pollEventLoop(this);
      });
    #elif defined(__ANDROID__) || !defined(__linux__)
      Lock lock(this->mutex);
      this->thread = new std::thread(&pollEventLoop, this);
    #endif

    return *this;
  }

  Loop& Loop::signal () {
    this->semaphores.signal.acquire();
    uv_async_send(&this->async);
    this->semaphores.signal.release();
    return *this;
  }

  Loop& Loop::dispatch (DispatchCallback callback) {
    if (callback != nullptr) {
      Lock lock(this->mutex);
      this->queue.push(callback);
    }

    this->start();
    this->signal();
    return *this;
  }

  void pollEventLoop (Loop* loop) {
    loop->semaphores.signal.release();

    while (loop->isRunning()) {
      loop->sleep(Loop::POLL_TIMEOUT);

      do {
        loop->run();
      } while (loop->isRunning() && loop->isAlive());
    }

    loop->semaphores.poll.release();
    loop->running = false;
  }
}
