#ifndef KOBOPLATFORMADDITIONS_H
#define KOBOPLATFORMADDITIONS_H

#include <QObject>

#include "einkenums.h"
#include "kobodevicedescriptor.h"

class KoboPlatformAdditions : public QObject
{
    Q_OBJECT

public:
    KoboPlatformAdditions(QObject *parent, const KoboDeviceDescriptor &device);

    int getBatteryLevel() const;
    bool isBatteryCharging() const;

    bool isUsbConnected() const;
    void setStatusLedEnabled(bool enabled);

    void setFrontlightLevel(int val, int temp);

private:
    void setNaturalBrightness(int brig, int temp);

private:
    KoboDeviceDescriptor device;
};

#endif  // KOBOPLATFORMADDITIONS_H
