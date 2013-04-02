#ifndef IIODEVICE_H
#define IIODEVICE_H

#include <QObject>
#include <QSharedPointer>
#include <QHash>
#include <QMap>
#include <QSocketNotifier>
#include <QTimer>

/* Manage a kernel IIO device, potentially shared between adaptors
 * for chips with several sensors. */
class IioDevice : public QObject
{
    Q_OBJECT

public:
    /* Find the first IioDevice with a given element, e.g. in_accel_x */
    static QSharedPointer<IioDevice> findDeviceByElement(const QString &element);

    virtual ~IioDevice();

    bool setElementEnabled(const QString &element, bool enabled);

    /* Retrieve the most recent value for a given element. Watch the
     * dataReady signal for changes. */
    qint64 getElementData(const QString &element);

signals:
    void dataReady();

private slots:
    bool openDevice();
    void deviceReadable();

private:
    static QHash<QString, QWeakPointer<IioDevice> > devices;

    class ElementData;
    IioDevice(const QString &deviceName);

    QString deviceName, sysfsPath;
    QHash<QString,ElementData*> elements;
    QMap<int,ElementData*> orderedElements;
    int scanSize;
    int deviceFd;
    QSocketNotifier *notifier;
    QTimer delayedOpenTimer;

    void closeDevice();
    void scanElements();

    QString readFile(const QString &path);
    bool writeFile(const QString &path, const QString &value);
};

#endif
