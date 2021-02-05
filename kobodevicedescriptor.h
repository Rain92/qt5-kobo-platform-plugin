#ifndef KOBODEVICEDESCRIPTOR_H
#define KOBODEVICEDESCRIPTOR_H

#include <QtCore>
#include <iostream>

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
    KoboLibra,
    KoboOther
};

struct TouchscreenTransform
{
    int rotation;
    bool invertX;
    bool invertY;
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
    TouchscreenTransform touchscreenTransform;
    bool hasComfortLight;
    int frontlightMaxLevel;
    int frontlightMaxTemp;
};

KoboDeviceDescriptor determineDevice();

#endif  // KOBODEVICEDESCRIPTOR_H
