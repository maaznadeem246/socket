#ifndef SSC_CORE_BLUETOOTH_HH
#define SSC_CORE_BLUETOOTH_HH

#include "data.hh"
#include "ipc.hh"
#include "json.hh"
#include "types.hh"

namespace ssc::core::bluetooth {
  using namespace ssc::core::types;
  using namespace ssc::core::data;

  // forward
  class CoreBluetoothInternals;

  class CoreBluetooth {
    public:
      CoreBluetoothInternals* internals = nullptr;
      ipc::IRouter& router;

      CoreBluetooth (ipc::IRouter&);
      ~CoreBluetooth ();
      bool send (const String& seq, JSON::Any json, CoreData data);
      bool send (const String& seq, JSON::Any json);
      bool emit (const String& seq, JSON::Any json);
      void startScanning ();
      void publishCharacteristic (
        const String& seq,
        char* bytes,
        size_t size,
        const String& serviceId,
        const String& characteristicId
      );
      void subscribeCharacteristic (
        const String& seq,
        const String& serviceId,
        const String& characteristicId
      );
      void startService (
        const String& seq,
        const String& serviceId
      );
  };
}

#endif
