#include <socket/socket.hh>
#import <UIKit/UIKit.h>

#include "application.hh"

#if defined(__APPLE__) && (TARGET_OS_IPHONE || TARGET_IPHONE_SIMULATOR)
@implementation IOSApplication
@end
#endif

int main (const int argc, const char **argv) {
  ssc::ios::Application app(argc, argv);
  @autoreleasepool {
    return UIApplicationMain(argc, (char **) argv, nil, NSStringFromClass([IOSApplication class]));
  }
}
