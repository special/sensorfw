#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdio.h>
#include <QDebug>
#include <QFile>

#include "config.h"
#include "mpuaccelerometeradaptor.h"
#include "logging.h"
#include "datatypes/utils.h"

MpuAccelAdaptor::MpuAccelAdaptor(const QString& id)
    : DeviceAdaptor(id)
{
    buffer = new DeviceAdaptorRingBuffer<OrientationData>(128);
    setAdaptedSensor("accelerometer", "mpu accelerometer", buffer);
    setDescription("mpu accelerometer");

    introduceAvailableDataRange(DataRange(-32768, 32767, 1));
}

MpuAccelAdaptor::~MpuAccelAdaptor()
{
    delete buffer;
}

void MpuAccelAdaptor::init()
{
    sensordLogW() << "MPU init";
    introduceAvailableDataRanges(name());
    introduceAvailableIntervals(name());
}

bool MpuAccelAdaptor::startAdaptor()
{
    sensordLogW() << "Starting adaptor: " << id();
    return true;
}

void MpuAccelAdaptor::stopAdaptor()
{
    sensordLogW() << "Stopping adaptor: " << id();
    if ( getAdaptedSensor()->isRunning() )
        stopSensor();
}

bool MpuAccelAdaptor::startSensor()
{
    AdaptedSensorEntry *entry = getAdaptedSensor();

    if (entry == NULL) {
        sensordLogW() << "Sensor not found: " << name();
        return false;
    }

    // Increase listener count
    entry->addReference();

    iioDevice = IioDevice::findDeviceByElement("in_accel_x");
    if (iioDevice.isNull())
        return false;

    iioDevice->setElementEnabled("in_accel_x", true);
    iioDevice->setElementEnabled("in_accel_y", true);
    iioDevice->setElementEnabled("in_accel_z", true);
    iioDevice->setElementEnabled("in_timestamp", true);
    connect(iioDevice.data(), SIGNAL(dataReady()), SLOT(dataReady()));

    entry->setIsRunning(true);
    return true;
}

void MpuAccelAdaptor::stopSensor()
{
    AdaptedSensorEntry *entry = getAdaptedSensor();

    if (entry == NULL) {
        sensordLogW() << "Sensor not found " << name();
        return;
    }

    entry->removeReference();
    if (entry->referenceCount() <= 0) {
        entry->setIsRunning(false);
    }
 
    iioDevice.clear();
}

void MpuAccelAdaptor::dataReady()
{
    int x, y, z;
    qlonglong ts;

    if (iioDevice.isNull())
        return;

    x = iioDevice->getElementData("in_accel_x");
    y = iioDevice->getElementData("in_accel_y");
    z = iioDevice->getElementData("in_accel_z");
    ts = iioDevice->getElementData("in_timestamp");

    AccelerationData *d = buffer->nextSlot();

    d->timestamp_ = ts / 1000;
    d->x_ = x;
    d->y_ = y;
    d->z_ = -z;

    buffer->commit();
    buffer->wakeUpReaders();
}

