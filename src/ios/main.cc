#include <socket/socket.hh>
#import <UIKit/UIKit.h>

#include "application.hh"

int main (const int argc, const char **argv) {
  ssc::ios::Application app(argc, argv);
  @autoreleasepool {
    return UIApplicationMain(argc, (char **) argv, nil, NSStringFromClass([IOSApplication class]));
  }
}
