#include "iiodevice.h"
#include "logging.h"
#include <QDir>
#include <QDebug>
#include <qendian.h>
#include <errno.h>
#include <sys/types.h>
#include <fcntl.h>

class IioDevice::ElementData
{
public:
    QString name;
    int index;
    bool enabled;

    qint64 value;
    unsigned location;

    bool bigEndian, sign;
    unsigned bytes;
    unsigned shift;
    quint64 mask;

    ElementData(const QString &name, int index, bool enabled, const QString &type)
        : name(name), index(index), enabled(enabled), value(0), location(0),
          bigEndian(false), sign(false), bytes(0), shift(0), mask(0)
    {
        char e, s;
        unsigned bits = 0, pad = 0;
        sscanf(type.toLatin1().constData(), "%ce:%c%u/%u>>%u", &e, &s, &bits, &pad, &shift);

        bigEndian = (e == 'b');
        sign = (s == 's');
        bytes = pad / 8;

        if (bits == 64)
            mask = ~0;
        else
            mask = (1 << bits) - 1;
    }
};

QHash<QString, QWeakPointer<IioDevice> > IioDevice::devices;

static const char *sysfsRoot = "/sys/bus/iio/devices/";

QSharedPointer<IioDevice> IioDevice::findDeviceByElement(const QString &element)
{
    QDir iioDir(sysfsRoot);
    QStringList deviceNames = iioDir.entryList(QStringList() << "iio:device*");

    foreach (const QString &deviceName, deviceNames) {
        QDir elementsDir(sysfsRoot + deviceName + "/scan_elements/");
        if (!elementsDir.exists(element + "_index"))
            continue;

        QSharedPointer<IioDevice> re;

        QHash<QString,QWeakPointer<IioDevice> >::iterator it = devices.find(deviceName);
        if (it != devices.end())
            re = it->toStrongRef();

        if (re.isNull()) {
            re = QSharedPointer<IioDevice>(new IioDevice(deviceName));
            devices.insert(deviceName, re.toWeakRef());
        }

        return re;
    }

    return QSharedPointer<IioDevice>();
}

IioDevice::IioDevice(const QString &name)
    : deviceName(name), scanSize(0), deviceFd(-1), notifier(0)
{
    sysfsPath = sysfsRoot + name + "/";
    qDebug() << Q_FUNC_INFO << sysfsPath;

    delayedOpenTimer.setSingleShot(true);
    delayedOpenTimer.setInterval(0);
    connect(&delayedOpenTimer, SIGNAL(timeout()), SLOT(openDevice()));
}

IioDevice::~IioDevice()
{
    closeDevice();
    qDeleteAll(elements);
}

bool IioDevice::openDevice()
{
    if (deviceFd >= 0)
        return true;

    qDebug() << Q_FUNC_INFO;

    // Reload list of elements for reading data
    scanElements();

    // XXX Configurable
    if (!writeFile(sysfsPath + "buffer/length", "200") ||
        !writeFile(sysfsPath + "buffer/enable", "1")) {
        sensordLogW() << "Cannot enable IIO buffer";
        return false;
    }

    QString path = "/dev/" + deviceName;
    deviceFd = open(path.toLatin1().constData(), O_RDONLY);
    if (deviceFd < 0) {
        sensordLogW() << "Cannot read" << path << "-" << strerror(errno);
        return false;
    }

    notifier = new QSocketNotifier(deviceFd, QSocketNotifier::Read, this);
    connect(notifier, SIGNAL(activated(int)), SLOT(deviceReadable()));
    notifier->setEnabled(true);
    return true;
}

void IioDevice::closeDevice()
{
    if (deviceFd < 0)
        return;

    close(deviceFd);
    deviceFd = -1;

    delete notifier;
    notifier = 0;

    qDeleteAll(elements);
    elements.clear();
    orderedElements.clear();
    scanSize = 0;

    writeFile(sysfsPath + "buffer/enable", "0");
}

void IioDevice::scanElements()
{
    QDir elementsDir(sysfsPath + "scan_elements/");

    // Find all elements, and keep a list (sorted by index) of enabled elements
    QStringList names = elementsDir.entryList(QStringList() << "in_*_index");
    foreach (QString name, names) {
        // Remove _index
        name.chop(6);

        int index = readFile(elementsDir.filePath(name + "_index")).toInt();
        bool enabled = readFile(elementsDir.filePath(name + "_en")).toInt() ? true : false;
        QString type = readFile(elementsDir.filePath(name + "_type"));

        if (!enabled || type.isEmpty())
            continue;

        ElementData *d = new ElementData(name, index, enabled, type);
        elements.insert(name, d);

        orderedElements.insert(d->index, d);
    }

    // Calculate the size and position of a line of data from the device.
    // The line has all enabled elements in index order, with padding applied
    // where necessary.
    scanSize = 0;
    foreach (ElementData *d, orderedElements) {
        if (scanSize % d->bytes)
            d->location = scanSize - scanSize % d->bytes + d->bytes;
        else
            d->location = scanSize;
        scanSize = d->location + d->bytes;
    }
}

bool IioDevice::setElementEnabled(const QString &element, bool enabled)
{
    if (enabled == elements.contains(element))
        return true;

    // Close the device and start a timer to reopen it. Elements are scanned when opening.
    closeDevice();
    delayedOpenTimer.start();

    if (!writeFile(sysfsPath + "scan_elements/" + element + "_en", enabled ? "1" : "0"))
        return false;

    qDebug() << Q_FUNC_INFO;

    return true;
}

qint64 IioDevice::getElementData(const QString &element)
{
    ElementData *d = elements.value(element);
    if (d)
        return d->value;
    return 0;
}

QString IioDevice::readFile(const QString &path)
{
    QFile f(path);
    if (!f.open(QIODevice::ReadOnly))
        return QString();

    return f.readAll();
}

bool IioDevice::writeFile(const QString &path, const QString &value)
{
    QFile f(path);
    if (!f.open(QIODevice::WriteOnly | QIODevice::Truncate))
        return false;

    if (f.write(value.toLatin1()) < 0)
        return false;
    return true;
}

void IioDevice::deviceReadable()
{
    if (!notifier || deviceFd < 0)
        return;

    notifier->setEnabled(false);

    uchar buf[scanSize * 32];
    ssize_t re = read(deviceFd, buf, sizeof(buf));
    if (re < 0) {
        if (errno != EAGAIN) {
            sensordLogW() << "Read from device" << deviceName << "failed:" << strerror(errno);
            // Return without re-enabling the notifier
        } else
            notifier->setEnabled(true);
        return;
    }
 
    int lineStart = 0;
    while (lineStart + scanSize <= re) {
        foreach (ElementData *d, orderedElements) {
            const uchar *p = buf + lineStart + d->location;

            switch (d->bytes) {
                case 2:
                    if (d->sign)
                        d->value = d->bigEndian ? qFromBigEndian<qint16>(p) : qFromLittleEndian<qint16>(p);
                    else
                        d->value = d->bigEndian ? qFromBigEndian<quint16>(p) : qFromLittleEndian<quint16>(p);
                    break;
                case 4:
                    if (d->sign)
                        d->value = d->bigEndian ? qFromBigEndian<qint32>(p) : qFromLittleEndian<qint32>(p);
                    else
                        d->value = d->bigEndian ? qFromBigEndian<quint32>(p) : qFromLittleEndian<quint32>(p);
                    break;
                case 8:
                    if (d->sign)
                        d->value = d->bigEndian ? qFromBigEndian<qint64>(p) : qFromLittleEndian<qint64>(p);
                    else
                        d->value = d->bigEndian ? qFromBigEndian<quint64>(p) : qFromLittleEndian<quint64>(p);
                    break;
            }
        }

        lineStart += scanSize;
        emit dataReady();
    }

    notifier->setEnabled(true);
}

