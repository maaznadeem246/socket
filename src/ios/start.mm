#include <socket/socket.hh>

#import <UIKit/UIKit.h>
#include "../core/internal/application.hh"
#include "start.hh"

namespace ssc {
  int start (const int argc, char** argv) {
    @autoreleasepool {
      return UIApplicationMain(argc, (char **) argv, nil, NSStringFromClass([IOSApplication class]));
    }
    return 0;
  }
}
