module; // global
#include "../platform.hh"

/**
 * @module ssc.loop
 * @description TODO
 * @example
 * TODO
 */
export module ssc.loop;
import ssc.uv;

export namespace ssc::loop {
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

  class Loop {
    public:
      static constexpr int POLL_TIMEOUT = 32; // in milliseconds
      using DispatchCallback = std::function<void()>;

      /**
       * An extened `GSource` container with a pointer to a tag created with
       * `g_source_add_unix_fd()` and the `Loop` instance that created it.
       * @see https://api.gtkd.org/glib.Source.Source.gSource.html
       */
      #if defined(__linux__) && !defined(__ANDROID__)
      struct LoopSource {
        GSource base; // should ALWAYS be first member
        gpointer tag;
        Loop* loop;
      };
      #endif

      // uv state
      uv_loop_t loop;
      uv_async_t async;
      // instance state
      AtomicBool initialized = false;
      AtomicBool running = false;
      // async state
      Queue<DispatchCallback> queue;
      Thread* thread = nullptr;
      Mutex mutex;

      #if defined(__APPLE__)
      dispatch_queue_t dispatchQueue = dispatch_queue_create(
        DISPATCH_QUEUE_LABEL,
        DISPATCH_QUEUE_QOS
      );
      #endif

      Loop () = default;
      Loop (Loop&) = delete;
      ~Loop () {
        this->stop();
      }

      uv_loop_t* get () {
        return &this->loop;
      }

      bool isInitialized () const {
        return this->initialized;
      }

      bool isAlive () {
        return this->isInitialized() && uv_loop_alive(&this->loop);
      }

      bool isRunning () const {
        return this->isInitialized() && this->running;
      }

      int timeout () {
        uv_update_time(&this->loop);
        return uv_backend_timeout(&this->loop);
      }

      int run (uv_run_mode mode) {
        return uv_run(&this->loop, mode);
      }

      int run () {
        return this->run(UV_RUN_DEFAULT);
      }

      void sleep (int64_t ms) {
        if (ms > 0) {
          auto timeout = this->timeout();
          ms = timeout > ms ? timeout : ms;
          std::this_thread::sleep_for(std::chrono::milliseconds(ms));
        }
      }

      void sleep () {
        this->sleep(this->timeout());
      }

      void signal () {
        uv_async_send(&this->async);
      }

      void dispatch (DispatchCallback callback) {
        Lock lock(this->mutex);
        this->queue.push(callback);
        this->signal();
      }

      void init () {
        if (this->initialized) {
          return;
        }

        Lock lock(this->mutex);

        /**
         * Below we configure a table of `GSourceFuncs` used for
         * polling and running the uv event loop along side the one
         * used by GTK/glib) on Linux.
         * @see https://api.gtkd.org/glib.c.types.GSourceFuncs.html
         */
        #if defined(__linux__) && !defined(__ANDROID__)
        static GSourceFuncs gSourceFuncs = {
          .prepare = [](auto source, auto timeout) {
            auto loop = reinterpret_cast<LoopSource *>(source)->loop;

            if (!loop->isAlive() || !loop->isRuning()) {
              return false;
            }

            *timeout = loop->timeout();
            return 0 == *timeout;
          },

          .dispatch = [](auto source, auto callback, auto user_data) {
            auto loop = reinterpret_cast<LoopSource *>(source)->loop;
            loop->run(UV_RUN_NOWAIT);
            uv_run(loop, UV_RUN_NOWAIT);
            return G_SOURCE_CONTINUE;
          }
        };
        #endif

        uv_loop_init(&this->loop);
        uv_async_init(&this->loop, &this->async, [](uv_async_t *handle) {
          auto loop = reinterpret_cast<Loop*>(handle->data);

          while (true) {
            Lock lock(loop->mutex);
            if (loop->queue.size() > 0) {
              auto dispatch = loop->queue.front();
              if (dispatch != nullptr) {
                dispatch();
              }

              loop->queue.pop();
            }
          }
        });

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
      }

      void stop () {
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
      }

      void start () {
        if (this->isRunning()) {
          return;
        }

        Lock lock(this->mutex);

        this->running = true;
        this->init();
        this->stop();
        this->dispatch([=, this]() {
          // TODO
          // initTimers();
          // startTimers();
        });

        #if defined(__APPLE__)
        dispatch_async(this->dispatchQueue, ^{
          pollEventLoop(this);
        });

        #elif defined(__ANDROID__) || !defined(__linux__)
        this->thread = new std::thread(&pollEventLoop, this);
        #endif
      }
  };

  void pollEventLoop (Loop* loop) {
    while (loop->isRunning()) {
      loop->sleep(Loop::POLL_TIMEOUT);

      do {
        loop->run();
      } while (loop->isRunning() && loop->isAlive());
    }

    loop->running = false;
  }
}
