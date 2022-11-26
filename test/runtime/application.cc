#include <socket/socket.hh>
#include <assert.h>

using namespace ssc::runtime;

class Application : public CoreApplication {
  public:
    Window* createDefaultWindow () {
      return nullptr;
    }

    Window* createWindow () {
      return nullptr;
    }

    void start () {
    }

    void stop () {
    }

    void onPause () {
    }

    void onResume () {
    }
};

int main (const int argc, const char** argv) {
  Application app;
  app.run();
  return 0;
}
