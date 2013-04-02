#ifndef MPUACCELEROMETERADPTOR_H
#define MPUACCELEROMETERADPTOR_H
#include "deviceadaptor.h"
#include "iiodevice.h"
#include <QString>
#include <QStringList>
#include <linux/input.h>
#include "deviceadaptorringbuffer.h"
#include "datatypes/orientationdata.h"
#include <QTime>

class MpuAccelAdaptor : public DeviceAdaptor {
    Q_OBJECT

public:
    static DeviceAdaptor* factoryMethod (const QString& id) {
        return new MpuAccelAdaptor (id);
    }
    MpuAccelAdaptor(const QString& id);
    ~MpuAccelAdaptor();

    void init();
    bool startAdaptor();
    void stopAdaptor();
    bool startSensor();
    void stopSensor();

protected:
    virtual bool setStandbyOverride(const bool override) { Q_UNUSED(override); return false; }

private slots:
    void dataReady();

private:
    DeviceAdaptorRingBuffer<OrientationData>* buffer;
    QSharedPointer<IioDevice> iioDevice;
};
#endif
