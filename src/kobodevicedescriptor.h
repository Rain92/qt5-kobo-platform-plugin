#ifndef KOBODEVICEDESCRIPTOR_H
#define KOBODEVICEDESCRIPTOR_H

#include <QtCore>
#include <iostream>

enum KoboDevice
{
    KoboUnknown,
    KoboTouchAB,
    KoboTouchC,
    KoboMini,
    KoboGlo,
    KoboGloHD,
    KoboTouch2,
    KoboAura,
    KoboAuraHD,
    KoboAuraH2O,
    KoboAuraH2O2_v1,
    KoboAuraH2O2_v2,
    KoboAuraOne,
    KoboAura2,
    KoboAura2_v2,
    KoboClaraHD,
    KoboForma,
    KoboLibraH2O,
    KoboNia,
    KoboElipsa
};

struct TouchscreenSettings
{
    bool swapXY = true;
    bool invertX = true;
    bool invertY = false;
    bool hasMultitouch = true;
};

struct FrontlightSettings
{
    bool hasFrontLight = true;
    int frontlightMin = 0;
    int frontlightMax = 100;
    bool hasNaturalLight = false;
    bool hasNaturalLightMixer = false;
    bool naturalLightInverted = false;
    int naturalLightMin = 0;
    int naturalLightMax = 100;

    QString frontlightDevWhite = "/sys/class/backlight/mxc_msp430.0/brightness";
    QString frontlightDevMixer = "/sys/class/backlight/lm3630a_led/color";
    QString frontlightDevRed = "/sys/class/backlight/lm3630a_led1a";
    QString frontlightDevGreen = "/sys/class/backlight/lm3630a_ledb";
};

struct KoboDeviceDescriptor
{
    KoboDevice device;
    QString modelName;
    int modelNumber;
    int mark = 6;
    int dpi;
    int width;
    int height;
    qreal physicalWidth;
    qreal physicalHeight;

    bool hasKeys = false;
    bool canToggleChargingLED = false;

    // New devices *may* be REAGL-aware, but generally don't expect explicit REAGL requests, default to not.
    bool isREAGL = false;

    // unused for now
    bool hasGSensor = false;

    TouchscreenSettings touchscreenSettings;
    FrontlightSettings frontlightSettings;

    // MXCFB_WAIT_FOR_UPDATE_COMPLETE ioctls are generally reliable
    bool hasReliableMxcWaitFor = true;
    // Sunxi devices require a completely different fb backend...
    bool isSunxi = false;

    // Standard sysfs path to the battery directory
    QString batterySysfs = "/sys/class/power_supply/mc13892_bat";
    // Stable path to the NTX input device
    QString ntxDev = "/dev/input/event0";
    // Stable path to the Touch input device
    QString touchDev = "/dev/input/event1";
};

KoboDeviceDescriptor determineDevice();

#endif  // KOBODEVICEDESCRIPTOR_H
