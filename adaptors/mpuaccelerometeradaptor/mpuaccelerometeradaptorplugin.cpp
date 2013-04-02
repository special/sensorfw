#include "mpuaccelerometeradaptorplugin.h"
#include "mpuaccelerometeradaptor.h"
#include "sensormanager.h"
#include "logging.h"

void MpuAccelerometerAdaptorPlugin::Register(class Loader&)
{
    sensordLogD() << "registering mpuaccelerometeradaptor";
    SensorManager& sm = SensorManager::instance();
    sm.registerDeviceAdaptor<MpuAccelAdaptor>("accelerometeradaptor");
}

Q_EXPORT_PLUGIN2(mpuaccelerometeradaptor,MpuAccelerometerAdaptorPlugin)
