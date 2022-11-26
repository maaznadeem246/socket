#include <socket/runtime.hh>

#if defined(__APPLE__)
#import <Network/Network.h>
#import <UserNotifications/UserNotifications.h>
#endif

using namespace ssc;

#if defined(__APPLE__)
@interface NetworkStatusMonitor : NSObject
@property (strong, nonatomic) NSObject<OS_dispatch_queue>* monitorQueue;
@property (nonatomic) ssc::runtime::NetworkStatusObserver* observer;
@property (retain) nw_path_monitor_t monitor;
- (id) init;
- (id) initWithObserver: (ssc::runtime::NetworkStatusObserver*) observer;
@end
#endif

namespace ssc::runtime {
  NetworkStatusObserver::NetworkStatusObserver () {
    #if defined(__APPLE__)
      this->internal = (__bridge void*) [[NetworkStatusMonitor alloc] initWithObserver: this];
    #endif
  }

  NetworkStatusObserver::~NetworkStatusObserver () {
    #if defined(__APPLE__)
      if (this->internal != nullptr) {
        auto monitor = (__bridge NetworkStatusMonitor*) this->internal;
        #if !__has_feature(objc_arc)
          [monitor release];
        #endif
      }
    #endif
  }

  void NetworkStatusObserver::onNetworkStatusChange (
    const String& statusName,
    const String& message
  ) const {
    if (this->onNetworkStatusChangeCallback != nullptr) {
      this->onNetworkStatusChangeCallback(statusName, message);
    }
  }
}

#if defined(__APPLE__)
@implementation NetworkStatusMonitor
- (id) initWithObserver: (ssc::runtime::NetworkStatusObserver*) observer {
  self = [self init];
  self.observer = observer;
  return self;
}

- (id) init {
  self = [super init];
  dispatch_queue_attr_t attrs = dispatch_queue_attr_make_with_qos_class(
    DISPATCH_QUEUE_SERIAL,
    QOS_CLASS_UTILITY,
    DISPATCH_QUEUE_PRIORITY_DEFAULT
  );

  _monitorQueue = dispatch_queue_create(
    "co.socketsupply.queue.network-status-observer",
    attrs
  );

  // self.monitor = nw_path_monitor_create_with_type(nw_interface_type_wifi);
  _monitor = nw_path_monitor_create();
  nw_path_monitor_set_queue(self.monitor, self.monitorQueue);
  nw_path_monitor_set_update_handler(self.monitor, ^(nw_path_t path) {
    nw_path_status_t status = nw_path_get_status(path);

    String statusName;
    String message;

    switch (status) {
      case nw_path_status_invalid: {
        statusName = "offline";
        message = "Network path is invalid";
        break;
      }

      case nw_path_status_satisfied: {
        statusName = "online";
        message = "Network is usable";
        break;
      }

      case nw_path_status_satisfiable: {
        statusName = "online";
        message = "Network may be usable";
        break;
      }

      case nw_path_status_unsatisfied: {
        statusName = "offline";
        message = "Network is not usable";
        break;
      }
    }

    dispatch_async(dispatch_get_main_queue(), ^{
      self.observer->onNetworkStatusChange(statusName, message);
    });
  });

  nw_path_monitor_start(self.monitor);

  return self;
}

- (void) userNotificationCenter: (UNUserNotificationCenter *) center
        willPresentNotification: (UNNotification *) notification
          withCompletionHandler: (void (^)(UNNotificationPresentationOptions options)) completionHandler
{
  completionHandler(UNNotificationPresentationOptionList | UNNotificationPresentationOptionBanner);
}
@end
#endif
