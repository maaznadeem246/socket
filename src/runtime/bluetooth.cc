#include <socket/runtime.hh>
#include <socket/utils.hh>

using namespace ssc;
using namespace ssc::runtime;
using namespace ssc::utils;

#if defined(__APPLE__)
@interface BluetoothController : NSObject<
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
@property (nonatomic) BluetoothInternals* internals;
- (void) startAdvertising;
- (void) startScanning;
- (id) init;
@end
#endif

namespace ssc::runtime {
  class BluetoothInternals {
    public:
    #if defined(__APPLE__)
      BluetoothController* controller = nullptr;
      Bluetooth* bluetooth = nullptr;
    #endif
      BluetoothInternals (Bluetooth* bluetooth);
      ~ BluetoothInternals ();
  };
}

namespace ssc::runtime {
  BluetoothInternals::BluetoothInternals (Bluetooth* bluetooth) {
    this->bluetooth = bluetooth;
    #if defined(__APPLE__)
      this->controller = [BluetoothController new];
      this->controller.internals = this;
    #endif
  }

  BluetoothInternals::~BluetoothInternals () {
    #if defined(__APPLE__)
      if (this->controller != nullptr) {
        #if !__has_feature(objc_arc)
          [this->controller release];
        #endif
        this->controller = nullptr;
      }
    #endif
  }

  Bluetooth::Bluetooth (ipc::Router* router)
    : router(router)
  {
    this->internals = new BluetoothInternals(this);
  }

  Bluetooth::~Bluetooth () {
    if (this->internals != nullptr) {
      delete this->internals;
    }

    this->internals = nullptr;
  }

  bool Bluetooth::send (const String& seq, JSON::Any json, Data data) {
    return this->router->send(seq, json, data);
  }

  bool Bluetooth::send (const String& seq, JSON::Any json) {
    return this->router->send(seq, json, Data{});
  }

  bool Bluetooth::emit (const String& seq, JSON::Any json) {
    return this->router->emit(seq, json);
  }

  void Bluetooth::startScanning () {
    #if defined(__APPLE__)
      if (
        this->internals != nullptr &&
        this->internals->controller != nullptr
      ) {
        [this->internals->controller startScanning];
      }
    #endif
  }

  void Bluetooth::publishCharacteristic (
    const String &seq,
    char *bytes,
    size_t size,
    const String &serviceId,
    const String &characteristicId
  ) {
      if (serviceId.size() != 36) {
        this->send(seq, JSON::Object::Entries {
          {"source", "bluetooth.publish"},
          {"err", JSON::Object::Entries {
            {"message", "Invalid service id"}
          }}
        });
        return;
      }

      if (characteristicId.size() != 36) {
        this->send(seq, JSON::Object::Entries {
          {"source", "bluetooth.publish"},
          {"err", JSON::Object::Entries {
            {"message", "Invalid characteristic id"}
          }}
        });
        return;
      }

      if (bytes == nullptr || size <= 0) {
        this->send(seq, JSON::Object::Entries {
          {"source", "bluetooth.publish"},
          {"err", JSON::Object::Entries {
            {"message", "Missing bytes in publish"}
          }}
        });
        return;
      }

    #if defined(__APPLE__)
      auto ssid = [NSString stringWithUTF8String: serviceId.c_str()];
      auto scid = [NSString stringWithUTF8String: characteristicId.c_str()];

      auto controller = this->internals->controller;
      auto peripheralManager = [controller peripheralManager];
      auto characteristics = [controller characteristics];
      auto serviceMap = [controller serviceMap];
      auto cUUID = [CBUUID UUIDWithString: scid];

      if (!serviceMap[ssid]) {
        auto json = JSON::Object::Entries {
          {"source", "bluetooth.publish"},
          {"err", JSON::Object::Entries {
            {"message", "Invalid service id"}
          }}
        };

        this->send(seq, json);
        return;
      }

      if (![serviceMap[ssid] containsObject: cUUID]) {
        auto json = JSON::Object::Entries {
          {"source", "bluetooth.publish"},
          {"err", JSON::Object::Entries {
            {"message", "Invalid characteristic id"}
          }}
        };

        this->send(seq, json);
        return;
      }

      [controller startScanning]; // noop if already scanning.

      if (size == 0) {
        debug("CoreBluetooth: characteristic added");
        return;
      }

      // TODO (@heapwolf) enforce max message legth and split into multiple writes.
      // NSInteger amountToSend = self.dataToSend.length - self.sendDataIndex;
	    // if (amountToSend > 512) amountToSend = 512;
      auto data = [NSData dataWithBytes: bytes length: size];

      CBMutableCharacteristic* characteristic;
      for (CBMutableCharacteristic* ch in characteristics[ssid]) {
        if ([ch.UUID isEqual: cUUID]) characteristic = ch;
      }

      characteristic.value = data;

      auto didWrite = [peripheralManager
                updateValue: data
          forCharacteristic: characteristic
        onSubscribedCentrals: nil
      ];

      if (!didWrite) {
        debug("CoreBluetooth: did not write");
        return;
      }

      debug("CoreBluetooth: did write '%@' %@", data, characteristic);
    #else
      this->send(seq, JSON::Object::Entries {
        {"source", "bluetooth.publish"},
        {"err", JSON::Object::Entries {
          {"type", "NotSupportedError"},
          {"message", "Not supported"}
        }}
      });
    #endif
  }

  void Bluetooth::subscribeCharacteristic (
    const String &seq,
    const String &serviceId,
    const String &characteristicId
  ) {
    auto json = JSON::Object::Entries {
      {"err", JSON::Object::Entries {
        {"message", "Failed to subscribe to characteristic"},
        {"serviceId", serviceId},
      }}
    };

    #if defined(__APPLE__)
      auto ssid = [NSString stringWithUTF8String: serviceId.c_str()];
      auto scid = [NSString stringWithUTF8String: characteristicId.c_str()];

      auto controller = this->internals->controller;
      auto peripheralManager = [controller peripheralManager];
      auto characteristics = [controller characteristics];
      auto serviceMap = [controller serviceMap];
      auto services = [controller services];

      auto  ch = [[CBMutableCharacteristic alloc]
        initWithType: [CBUUID UUIDWithString: scid]
          properties: (CBCharacteristicPropertyNotify | CBCharacteristicPropertyRead | CBCharacteristicPropertyWrite)
              value: nil
        permissions: (CBAttributePermissionsReadable | CBAttributePermissionsWriteable)
      ];

      if (!characteristics[ssid]) {
        characteristics[ssid] = [NSMutableSet setWithObject: ch];
      } else {
        [characteristics[ssid] addObject: ch];
      }

      if (!serviceMap[ssid]) {
        serviceMap[ssid] = [NSMutableSet setWithObject: [CBUUID UUIDWithString: scid]];
      } else {
        [serviceMap[ssid] addObject: [CBUUID UUIDWithString: scid]];
      }

      CBMutableService* service = services[ssid];

      if (!service) {
        auto sUUID = [CBUUID UUIDWithString: ssid];
        service = [[CBMutableService alloc] initWithType: sUUID primary: YES];

        service.characteristics = characteristics[ssid];

        [services setValue: service forKey: ssid];
        [peripheralManager addService: service];
      }

      json = JSON::Object::Entries {
        {"data", JSON::Object::Entries {
          {"serviceId", serviceId},
          {"characteristicId", characteristicId},
          {"message", "Subscribed to characteristic"}
        }}
      };
    #endif

    this->send(seq, json);
  }

  void Bluetooth::startService (
    const String &seq,
    const String &serviceId
  ) {
    auto json = JSON::Object::Entries {
      {"err", JSON::Object::Entries {
        {"serviceId", serviceId},
        {"message", "Failed to start service"}
      }}
    };

    #if defined(__APPLE__)
      auto controller = this->internals->controller;
      if (controller != nullptr) {
        [controller startScanning];
        json = JSON::Object::Entries {
          {"data", JSON::Object::Entries {
            {"serviceId", serviceId}, {"message", "Service started"}
          }}
        };
      }
    #endif

    this->send(seq, json);
  }
}

#if defined(__APPLE__)
@implementation BluetoothController
- (id) init {
  self = [super init];

  _services = [[NSMutableDictionary alloc] init];
  _serviceMap = [[NSMutableDictionary alloc] init];
  _peripherals = [[NSMutableArray alloc] init];
  _characteristics = [[NSMutableDictionary alloc] init];

  _centralManager = [[CBCentralManager alloc]
    initWithDelegate: self
               queue: nil
  ];

  _peripheralManager = [[CBPeripheralManager alloc]
    initWithDelegate: self
               queue: nil
             options: nil
  ];

  return self;
}

// - (void) disconnect {
// }

// - (void) updateRssi {
//  debug("CoreBluetooth: updateRssi");
// }

- (void) startAdvertising {
  if (_peripheralManager.isAdvertising) return;

  NSArray* keys = [_serviceMap allKeys];
  if ([keys count] == 0) return;


  NSMutableArray* uuids = [[NSMutableArray alloc] init];
  for (NSString* key in keys) {
    [uuids addObject: [CBUUID UUIDWithString: key]];
  }

  [_peripheralManager startAdvertising: @{CBAdvertisementDataServiceUUIDsKey: [uuids copy]}];
}

- (void) stopAdvertising {
  debug("CoreBluetooth: stopAdvertising");

  [self.peripheralManager stopAdvertising];
}

- (void) startScanning {
  NSArray* keys = [_serviceMap allKeys];
  if ([keys count] == 0) return;

  NSMutableArray* uuids = [[NSMutableArray alloc] init];

  for (NSString* key in keys) {
    [uuids addObject: [CBUUID UUIDWithString: key]];
  }

  debug("CoreBluetooth: startScanning %@", uuids);

  [_centralManager
    scanForPeripheralsWithServices: [uuids copy]
    options: @{CBCentralManagerScanOptionAllowDuplicatesKey: @(YES)}
  ];
}

- (void) peripheralManagerDidUpdateState: (CBPeripheralManager*) peripheral {
  String state = "Unknown state";
  String message = "Unknown state";

  switch (peripheral.state) {
    case CBManagerStatePoweredOff:
      message = "CoreBluetooth BLE hardware is powered off.";
      state = "CBManagerStatePoweredOff";
      break;

    case CBManagerStatePoweredOn:
      // [self startAdvertising];
      [self startScanning];
      message = "CoreBluetooth BLE hardware is powered on and ready.";
      state = "CBManagerStatePoweredOn";
      break;

    case CBManagerStateUnauthorized:
      message = "CoreBluetooth BLE state is unauthorized.";
      state = "CBManagerStateUnauthorized";
      break;

    case CBManagerStateUnknown:
      message = "CoreBluetooth BLE state is unknown.";
      state = "CBManagerStateUnknown";
      break;

    case CBManagerStateUnsupported:
      message = "CoreBluetooth BLE hardware is unsupported on this platform.";
      state = "CBManagerStateUnsupported";
      break;

    default:
      break;
  }

  auto json = JSON::Object::Entries {
    {"data", JSON::Object::Entries {
      {"event", "state"},
      {"state", message}
    }}
  };

  self.internals->bluetooth->emit("bluetooth", JSON::Object(json).str());
}

- (void) peripheralManagerDidStartAdvertising: (CBPeripheralManager*) peripheral
                                        error: (NSError*) error
{
  if (error) {
    debug("CoreBluetooth: %@", error);
    auto desc = String([error.debugDescription UTF8String]);
    std::replace(desc.begin(), desc.end(), '"', '\''); // Secure

    auto json = JSON::Object::Entries {
      {"err", JSON::Object::Entries {
        {"event", "peripheralManagerDidStartAdvertising"},
        {"message", desc}
      }}
    };

    self.internals->bluetooth->emit("bluetooth", JSON::Object(json).str());
    return;
  }

  debug("CoreBluetooth didStartAdvertising: %@", _serviceMap);
}

-    (void) peripheralManager: (CBPeripheralManager*) peripheralManager
                      central: (CBCentral*) central
 didSubscribeToCharacteristic: (CBCharacteristic*) characteristic
{
  debug("CoreBluetooth: didSubscribeToCharacteristic");
}

- (void) centralManagerDidUpdateState: (CBCentralManager*) central {
  switch (central.state) {
    case CBManagerStatePoweredOff:
    case CBManagerStateResetting:
    case CBManagerStateUnauthorized:
    case CBManagerStateUnknown:
    case CBManagerStateUnsupported:
      break;

    case CBManagerStatePoweredOn:
      [self startScanning];
    break;
  }
}

- (void) peripheralManager: (CBPeripheralManager*) peripheral
             didAddService: (CBService*) service
                     error: (NSError *) error
{
  [self startAdvertising];
}

- (void) peripheralManager: (CBPeripheralManager*) peripheral
     didReceiveReadRequest: (CBATTRequest*) request
{
  CBMutableCharacteristic* ch;

  for (NSString* key in _services) {
    NSMutableSet* characteristics = _characteristics[key];

    for (CBMutableCharacteristic* c in characteristics) {
      if (c.UUID == request.characteristic.UUID) {
        ch = c;
        break;
      }
    }
  }

  if (!ch) {
    debug("CoreBluetooth: No local characteristic found for request");
    return;
  }

  request.value = ch.value;
  [_peripheralManager respondToRequest: request withResult: CBATTErrorSuccess];

  [self startScanning];
}

- (void) centralManager: (CBCentralManager*) central
  didDiscoverPeripheral: (CBPeripheral*) peripheral
      advertisementData: (NSDictionary*) advertisementData
                   RSSI: (NSNumber*) RSSI
{
  if (peripheral.identifier == nil || peripheral.name == nil) {
    [self.peripherals addObject: peripheral];

    NSTimeInterval _scanTimeout = 0.5;
    [NSTimer timerWithTimeInterval: _scanTimeout repeats: NO block:^(NSTimer* timer) {
      debug("CoreBluetooth: reconnecting");
      [self->_centralManager connectPeripheral: peripheral options: nil];
    }];
    return;
  }

  auto isConnected = peripheral.state != CBPeripheralStateDisconnected;
  auto isKnown = [_peripherals containsObject: peripheral];

  if (isKnown && isConnected) {
    return;
  }

  if (!isKnown) {
    peripheral.delegate = self;
    [self.peripherals addObject: peripheral];
  }

  [_centralManager connectPeripheral: peripheral options: nil];

  for (CBService *service in peripheral.services) {
    NSString* key = service.UUID.UUIDString;
    NSArray* characteristics = [_characteristics[key] allObjects];
    for (CBCharacteristic* ch in characteristics) {
      [peripheral readValueForCharacteristic: ch];
    }
  }
}

- (void) centralManager: (CBCentralManager*) central
   didConnectPeripheral: (CBPeripheral*) peripheral
{
  debug("CoreBluetooth: didConnectPeripheral");
  peripheral.delegate = self;

  NSArray* keys = [_serviceMap allKeys];
  NSMutableArray* uuids = [[NSMutableArray alloc] init];

  for (NSString* key in keys) {
    [uuids addObject: [CBUUID UUIDWithString: key]];
  }

  String uuid = String([peripheral.identifier.UUIDString UTF8String]);
  String name = String([peripheral.name UTF8String]);

  if (uuid.size() == 0 || name.size() == 0) {
    debug("device has no meta information");
    return;
  }

  auto json = JSON::Object::Entries {
    {"data" , JSON::Object::Entries {
      {"event", "connect"},
      {"name", name},
      {"uuid", uuid}
    }}
  };

  self.internals->bluetooth->emit("bluetooth", JSON::Object(json).str());

  [peripheral discoverServices: uuids];
}

-  (void) peripheral: (CBPeripheral*) peripheral
 didDiscoverServices: (NSError*) error {
  if (error) {
    debug("CoreBluetooth: peripheral:didDiscoverService:error: %@", error);
    return;
  }

  for (CBService *service in peripheral.services) {
    NSString* key = service.UUID.UUIDString;
    NSArray* uuids = [_serviceMap[key] allObjects];

    debug("CoreBluetooth: peripheral:didDiscoverServices withChracteristics: %@", uuids);
    [peripheral discoverCharacteristics: [uuids copy] forService: service];
  }
}

-                   (void) peripheral: (CBPeripheral*) peripheral
 didDiscoverCharacteristicsForService: (CBService*) service
                                error: (NSError*) error
{
  if (error) {
    debug("CoreBluetooth: peripheral:didDiscoverCharacteristicsForService:error: %@", error);
    return;
  }

  NSString* key = service.UUID.UUIDString;
  NSArray* uuids = [[_serviceMap[key] allObjects] copy];

  for (CBCharacteristic* characteristic in service.characteristics) {
    for (CBUUID* cUUID in uuids) {
      if ([characteristic.UUID isEqual: cUUID]) {
        [peripheral setNotifyValue: YES forCharacteristic: characteristic];
        [peripheral readValueForCharacteristic: characteristic];
      }
    }
  }
}

- (void) peripheralManagerIsReadyToUpdateSubscribers: (CBPeripheralManager*) peripheral {
  auto json = JSON::Object::Entries {
    {"data", JSON::Object::Entries {
      {"source", "bluetooth"},
      {"message", "peripheralManagerIsReadyToUpdateSubscribers"},
      {"event", "status"}
    }}
  };

  self.internals->bluetooth->emit("bluetooth", JSON::Object(json).str());
}

-              (void) peripheral: (CBPeripheral*) peripheral
 didUpdateValueForCharacteristic: (CBCharacteristic*) characteristic
                           error: (NSError*) error
{
  if (error) {
    debug("ERROR didUpdateValueForCharacteristic: %@", error);
    return;
  }

  if (!characteristic.value || characteristic.value.length == 0) return;

  String uuid = "";
  String name = "";

  if (peripheral.identifier != nil) {
    uuid = [peripheral.identifier.UUIDString UTF8String];
  }

  if (peripheral.name != nil) {
    name = [peripheral.name UTF8String];
    std::replace(name.begin(), name.end(), '\n', ' '); // Secure
  }

  String characteristicId = [characteristic.UUID.UUIDString UTF8String];
  String sid = "";

  for (NSString* ssid in [_serviceMap allKeys]) {
    if ([_serviceMap[ssid] containsObject: characteristic.UUID]) {
      sid = [ssid UTF8String];
    }
  }

  auto bytes = (char*) characteristic.value.bytes;
  auto length = (int) characteristic.value.length;
  auto headers = Headers {{
    {"content-type", "application/octet-stream"},
    {"content-length", length}
  }};

  Data data = {0};
  data.id = rand64();
  data.body = bytes;
  data.length = length;
  data.headers = headers.str();

  auto json = JSON::Object::Entries {
    {"data", JSON::Object::Entries {
      {"event", "data"},
      {"source", "bluetooth"},
      {"characteristicId", characteristicId},
      {"serviceId", sid},
      {"name", name},
      {"uuid", uuid}
    }}
  };

  self.internals->bluetooth->send("-1", JSON::Object(json).str(), data);
}

-                          (void) peripheral: (CBPeripheral*) peripheral
 didUpdateNotificationStateForCharacteristic: (CBCharacteristic*) characteristic
                                       error: (NSError*) error
{
  // TODO
}

-     (void) centralManager: (CBCentralManager*) central
 didFailToConnectPeripheral: (CBPeripheral*) peripheral
                      error: (NSError*) error
{
  // if (error != nil) {
  //  debug("CoreBluetooth: failed to connect %@", error.debugDescription);
  //  return;
  // }

  NSTimeInterval _scanTimeout = 0.5;

  [NSTimer timerWithTimeInterval: _scanTimeout repeats: NO block:^(NSTimer* timer) {
    [self->_centralManager connectPeripheral: peripheral options: nil];

    // if ([_peripherals containsObject: peripheral]) {
    //  [_peripherals removeObject: peripheral];
    // }
  }];
}

-  (void) centralManager: (CBCentralManager*) central
 didDisconnectPeripheral: (CBPeripheral*) peripheral
                   error: (NSError*) error
{
  [_centralManager connectPeripheral: peripheral options: nil];
  if (error != nil) {
    debug("CoreBluetooth: device did disconnect %@", error.debugDescription);
    return;
  }
}
@end
#endif
