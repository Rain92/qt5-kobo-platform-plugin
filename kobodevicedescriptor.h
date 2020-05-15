#ifndef KOBODEVICEDESCRIPTOR_H
#define KOBODEVICEDESCRIPTOR_H
#include <QString>

enum KoboDevice
{
    KoboTouch = 1,
    KoboGlo,
    KoboMini,
    KoboAura,
    KoboAuraHD,
    KoboAuraH2O,
    KoboGloHD,
    KoboTouch2,
    KoboAura2,
    KoboAuraH2O2_v1,
    KoboAuraH2O2_v2,
    KoboAuraOne,
    KoboClaraHD,
    KoboForma,
    KoboOther
};

struct KoboDeviceDescriptor
{
    KoboDevice device;
    QString device_code_name;
    int model_number;
    int dpi;
    int width;
    int height;
    qreal physicalWidth;
    qreal physicalHeight;
    bool hasComfortLight;
    int frontlightMaxLevel;
    int frontlightMaxTemp;
};

#endif  // KOBODEVICEDESCRIPTOR_H
