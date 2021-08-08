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
    KoboAura2_v2,
    KoboAuraH2O2_v1,
    KoboAuraH2O2_v2,
    KoboAuraOne,
    KoboClaraHD,
    KoboForma,
    KoboLibra,
    KoboNia,
    KoboElypsa,
    KoboOther
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
    int dpi;
    int width;
    int height;
    qreal physicalWidth;
    qreal physicalHeight;

    // viewport to compensate for bezels; unsupported for now
    QRect viewport;

    bool hasKeys = false;
    bool canToggleChargingLED = false;

    // New devices *may* be REAGL-aware, but generally don't expect explicit REAGL requests, default to not.
    bool isREAGL = false;
    //  Mark 7 devices sport an updated driver.
    bool isMk7 = false;

    // unused for now
    bool hasGSensor = false;

    TouchscreenSettings touchscreenSettings;
    FrontlightSettings frontlightSettings;

    // MXCFB_WAIT_FOR_UPDATE_COMPLETE ioctls are generally reliable
    bool hasReliableMxcWaitFor = true;
    // Sunxi devices require a completely different fb backend...
    bool isSunxi = false;
    // On sunxi, "native" panel layout used to compute the G2D rotation handle (e.g., deviceQuirks.nxtBootRota
    // in FBInk).
    int bootRotation = 0;
    // Standard sysfs path to the battery directory
    QString batterySysfs = "/sys/class/power_supply/mc13892_bat";
    // Stable path to the NTX input device
    QString ntxDev = "/dev/input/event0";
    // Stable path to the Touch input device
    QString touchDev = "/dev/input/event1";
    // Event code to use to detect contact pressure
    QString pressureEvent = "";
};

KoboDeviceDescriptor determineDevice();

#endif  // KOBODEVICEDESCRIPTOR_H
