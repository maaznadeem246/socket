module;

#include "../platform.hh"

export module ssc.runtime:loop;

export namespace ssc {
  constexpr int EVENT_LOOP_POLL_TIMEOUT = 32; // in milliseconds

  #if defined(__linux__) && !defined(__ANDROID__)
    struct UVSource {
      GSource base; // should ALWAYS be first member
      gpointer tag;
      Runtime *runtime;
    };

    // @see https://api.gtkd.org/glib.c.types.GSourceFuncs.html
    static GSourceFuncs loopSourceFunctions = {
      .prepare = [](GSource *source, gint *timeout) -> gboolean {
        auto runtime = reinterpret_cast<UVSource *>(source)->runtime;
        if (!runtime->isLoopAlive() || !runtime->isLoopRunning) {
          return false;
        }

        *timeout = runtime->getEventLoopTimeout();
        return 0 == *timeout;
      },

      .dispatch = [](
        GSource *source,
        GSourceFunc callback,
        gpointer user_data
      ) -> gboolean {
        auto runtime = reinterpret_cast<UVSource *>(source)->runtime;
        Lock lock(runtime->loopMutex);
        auto loop = runtime->getEventLoop();
        uv_run(loop, UV_RUN_NOWAIT);
        return G_SOURCE_CONTINUE;
      }
    };
  #endif

  void Runtime::initEventLoop () {
    if (didLoopInit) {
      return;
    }

    didLoopInit = true;
    Lock lock(loopMutex);
    uv_loop_init(&eventLoop);
    eventLoopAsync.data = (void *) this;
    uv_async_init(&eventLoop, &eventLoopAsync, [](uv_async_t *handle) {
      auto runtime = reinterpret_cast<ssc::Runtime  *>(handle->data);
      while (true) {
        Lock lock(runtime->loopMutex);
        if (runtime->eventLoopDispatchQueue.size() == 0) break;
        auto dispatch = runtime->eventLoopDispatchQueue.front();
        if (dispatch != nullptr) dispatch();
        runtime->eventLoopDispatchQueue.pop();
      }
    });

#if defined(__linux__) && !defined(__ANDROID__)
    GSource *source = g_source_new(&loopSourceFunctions, sizeof(UVSource));
    UVSource *uvSource = (UVSource *) source;
    uvSource->runtime = this;
    uvSource->tag = g_source_add_unix_fd(
      source,
      uv_backend_fd(&eventLoop),
      (GIOCondition) (G_IO_IN | G_IO_OUT | G_IO_ERR)
    );

    g_source_attach(source, nullptr);
#endif
  }

  uv_loop_t* Runtime::getEventLoop () {
    initEventLoop();
    return &eventLoop;
  }

  int Runtime::getEventLoopTimeout () {
    auto loop = getEventLoop();
    uv_update_time(loop);
    return uv_backend_timeout(loop);
  }

  bool Runtime::isLoopAlive () {
    return uv_loop_alive(getEventLoop());
  }

  void Runtime::stopEventLoop() {
    isLoopRunning = false;
    uv_stop(&eventLoop);
#if defined(__APPLE__)
    // noop
#elif defined(__ANDROID__) || !defined(__linux__)
    Lock lock(loopMutex);
    if (eventLoopThread != nullptr) {
      if (eventLoopThread->joinable()) {
        eventLoopThread->join();
      }

      delete eventLoopThread;
      eventLoopThread = nullptr;
    }
#endif
  }

  void Runtime::sleepEventLoop (int64_t ms) {
    if (ms > 0) {
      auto timeout = getEventLoopTimeout();
      ms = timeout > ms ? timeout : ms;
      std::this_thread::sleep_for(std::chrono::milliseconds(ms));
    }
  }

  void Runtime::sleepEventLoop () {
    sleepEventLoop(getEventLoopTimeout());
  }

  void Runtime::signalDispatchEventLoop () {
    initEventLoop();
    runEventLoop();
    uv_async_send(&eventLoopAsync);
  }

  void Runtime::dispatchEventLoop (EventLoopDispatchCallback callback) {
    Lock lock(loopMutex);
    eventLoopDispatchQueue.push(callback);
    signalDispatchEventLoop();
  }

  void pollEventLoop (Runtime *runtime) {
    auto loop = runtime->getEventLoop();

    while (runtime->isLoopRunning) {
      runtime->sleepEventLoop(EVENT_LOOP_POLL_TIMEOUT);

      do {
        uv_run(loop, UV_RUN_DEFAULT);
      } while (runtime->isLoopRunning && runtime->isLoopAlive());
    }

    runtime->isLoopRunning = false;
  }

  void Runtime::runEventLoop () {
    if (isLoopRunning) {
      return;
    }

    isLoopRunning = true;

    initEventLoop();
    dispatchEventLoop([=, this]() {
      initTimers();
      startTimers();
    });

#if defined(__APPLE__)
    Lock lock(loopMutex);
    dispatch_async(eventLoopQueue, ^{ pollEventLoop(this); });
#elif defined(__ANDROID__) || !defined(__linux__)
    Lock lock(loopMutex);
    // clean up old thread if still running
    if (eventLoopThread != nullptr) {
      if (eventLoopThread->joinable()) {
        eventLoopThread->join();
      }

      delete eventLoopThread;
      eventLoopThread = nullptr;
    }

    eventLoopThread = new std::thread(&pollEventLoop, this);
#endif
  }
}
