#ifndef SSC_CORE_INTERNAL_BLUETOOTH_HH
#define SSC_CORE_INTERNAL_BLUETOOTH_HH

#include "../private.hh"
#include "../bluetooth.hh"

namespace ssc::core::bluetooth {
  // forward
  class CoreBluetoothInternals;
  class CoreBluetooth;
}

#if defined(__APPLE__)
@interface CoreBluetoothController : NSObject<
  CBCentralManagerDelegate,
  CBPeripheralManagerDelegate,
  CBPeripheralDelegate
>
@property (strong, nonatomic) CBCentralManager* centralManager;
@property (strong, nonatomic) CBPeripheralManager* peripheralManager;
@property (strong, nonatomic) CBPeripheral* bluetoothPeripheral;
@property (strong, nonatomic) NSMutableArray* peripherals;
@property (strong, nonatomic) NSMutableDictionary* services;
@property (strong, nonatomic) NSMutableDictionary* characteristics;
@property (strong, nonatomic) NSMutableDictionary* serviceMap;
@property (nonatomic) ssc::core::bluetooth::CoreBluetoothInternals* internals;
- (void) startAdvertising;
- (void) startScanning;
- (id) init;
@end
#endif

namespace ssc::core::bluetooth {
  class CoreBluetoothInternals {
    public:
    #if defined(__APPLE__)
      CoreBluetoothController* coreBluetoothController = nullptr;
      CoreBluetooth* coreBluetooth = nullptr;
    #endif
      CoreBluetoothInternals (CoreBluetooth* coreBluetooth);
      ~ CoreBluetoothInternals ();
  };
}
#endif
