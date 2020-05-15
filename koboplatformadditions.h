#ifndef H__KOBO_PLATFORM
#define H__KOBO_PLATFORM

#include <QObject>

#include "kobodevicedescriptor.h"
#include "kobokey.h"

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

#endif  // H__KOBO_PLATFORM
