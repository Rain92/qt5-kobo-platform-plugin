#include "kobodevicedescriptor.h"

#include <fcntl.h>
#include <linux/fb.h>
#include <private/qcore_unix_p.h>  // overrides QT_OPEN
#include <sys/ioctl.h>
#include <unistd.h>

// Kobo Touch A/B:
KoboDeviceDescriptor KoboTrilogyAB = {
    .device = KoboTouchAB,
    .mark = 3,
    .dpi = 200,
    .hasKeys = true,
    .touchscreenSettings{.swapXY = false, .hasMultitouch = false},
    .frontlightSettings = {.hasFrontLight = false},
};

// Kobo Touch C:
KoboDeviceDescriptor KoboTrilogyC = {
    .device = KoboTouchC,
    .mark = 4,
    .dpi = 200,
    .hasKeys = true,
    .touchscreenSettings{.swapXY = false, .hasMultitouch = false},
    .frontlightSettings = {.hasFrontLight = false},
};

// Kobo Mini:
KoboDeviceDescriptor KoboPixie = {
    .device = KoboMini,
    .mark = 4,
    .dpi = 200,
    .touchscreenSettings{.hasMultitouch = false},
    .frontlightSettings = {.hasFrontLight = false},
};

// Kobo Glo:
KoboDeviceDescriptor KoboKraken = {
    .device = KoboGlo,
    .mark = 4,
    .dpi = 212,
    .touchscreenSettings{.hasMultitouch = false},
};

// Kobo Glo HD:
KoboDeviceDescriptor KoboAlyssum = {
    .device = KoboGloHD,
    .mark = 6,
    .dpi = 300,
};

// Kobo Touch 2.0:
KoboDeviceDescriptor KoboPika = {
    .device = KoboTouch2,
    .mark = 6,
    .dpi = 167,
    .frontlightSettings = {.hasFrontLight = false},

};

// Kobo Aura:
KoboDeviceDescriptor KoboPhoenix = {
    .device = KoboAura,
    .mark = 5,
    .dpi = 212,
    // NOTE: AFAICT, the Aura was the only one explicitly requiring REAGL requests...
    .isREAGL = true,
};

// Kobo Aura HD:
KoboDeviceDescriptor KoboDragon = {
    .device = KoboAuraHD,
    .mark = 4,
    .dpi = 265,
    .touchscreenSettings{.hasMultitouch = false},
};

// Kobo Aura H2O:
KoboDeviceDescriptor KoboDahlia = {
    .device = KoboAuraH2O,
    .mark = 5,
    .dpi = 265,
};

// Kobo Aura H2O2:
KoboDeviceDescriptor KoboSnow = {
    .device = KoboAuraH2O2_v1,
    .mark = 6,
    .dpi = 265,
    .touchscreenSettings = {.invertX = false},
    .frontlightSettings =
        {
            .hasNaturalLight = true,
            .frontlightDevWhite = "/sys/class/backlight/lm3630a_ledb",
            .frontlightDevRed = "/sys/class/backlight/lm3630a_led",
            .frontlightDevGreen = "/sys/class/backlight/lm3630a_leda",
        },
};

// Kobo Aura H2O2, Rev2:
//- @fixme Check if the Clara fix actually helps here... (#4015)
KoboDeviceDescriptor KoboSnowRev2 = {
    .device = KoboAuraH2O2_v2,
    .mark = 7,
    .dpi = 265,
    .frontlightSettings =
        {
            .hasNaturalLight = true,
            .frontlightDevWhite = "/sys/class/backlight/lm3630a_ledb",
            .frontlightDevRed = "/sys/class/backlight/lm3630a_leda",
        },
};

// Kobo Aura One:
KoboDeviceDescriptor KoboDaylight = {
    .device = KoboAuraOne,
    .mark = 6,
    .dpi = 300,
    .frontlightSettings = {.hasNaturalLight = true,
                           .frontlightDevWhite = "/sys/class/backlight/lm3630a_led1b",
                           .frontlightDevRed = "/sys/class/backlight/lm3630a_led1a",
                           .frontlightDevGreen = "/sys/class/backlight/lm3630a_ledb"},
};

// Kobo Aura second edition:
KoboDeviceDescriptor KoboStar = {
    .device = KoboAura2,
    .mark = 6,
    .dpi = 212,
};

// Kobo Aura second edition, Rev 2:
KoboDeviceDescriptor KoboStarRev2 = {
    .device = KoboAura2_v2,
    .mark = 7,
    .dpi = 212,
};

// Kobo Clara HD:
KoboDeviceDescriptor KoboNova = {
    .device = KoboClaraHD,
    .mark = 7,
    .dpi = 300,
    .canToggleChargingLED = true,
    .frontlightSettings =
        {
            .hasNaturalLight = true,
            .hasNaturalLightMixer = true,
            .naturalLightInverted = true,
            .naturalLightMin = 0,
            .naturalLightMax = 10,
            .frontlightDevWhite = "/sys/class/backlight/mxc_msp430.0/brightness",
            .frontlightDevMixer = "/sys/class/backlight/lm3630a_led/color",
        },
};

// Kobo Forma:
// NOTE: Right now, we enforce Portrait orientation on startup to avoid getting touch coordinates wrong,
//       no matter the rotation we were started from (c.f., platform/kobo/koreader.sh).
// NOTE: For the FL, assume brightness is WO, and actual_brightness is RO!
//       i.e., we could have a real KoboPowerD:frontlightIntensityHW() by reading actual_brightness ;).
// NOTE: Rotation events *may* not be enabled if Nickel has never been brought up in that power cycle.
//       i.e., this will affect KSM users.
//       c.f., https://github.com/koreader/koreader/pull/4414#issuecomment-449652335
//       There's also a CM_ROTARY_ENABLE command, but which seems to do as much nothing as the STATUS one...
KoboDeviceDescriptor KoboFrost = {
    .device = KoboForma,
    .mark = 7,
    .dpi = 300,
    .hasKeys = true,
    .canToggleChargingLED = true,
    .hasGSensor = true,
    .frontlightSettings =
        {
            .hasNaturalLight = true,
            .hasNaturalLightMixer = true,
            // Warmth goes from 0 to 10 on the .device's side (our own internal scale is still normalized
            // to [0...100]) NOTE: Those three extra keys are *MANDATORY* if .frontlightDevMixer is set!
            .naturalLightInverted = true,
            .naturalLightMin = 0,
            .naturalLightMax = 10,
            .frontlightDevWhite = "/sys/class/backlight/mxc_msp430.0/brightness",
            .frontlightDevMixer = "/sys/class/backlight/tlc5947_bl/color",
        },
};

// Kobo Libra:
// NOTE: Assume the same quirks as the Forma apply.
KoboDeviceDescriptor KoboStorm = {
    .device = KoboLibraH2O,
    .mark = 7,
    .dpi = 300,
    .hasKeys = true,
    .canToggleChargingLED = true,
    .hasGSensor = true,
    .frontlightSettings =
        {
            .hasNaturalLight = true,
            .hasNaturalLightMixer = true,
            .naturalLightInverted = true,
            .naturalLightMin = 0,
            .naturalLightMax = 10,
            .frontlightDevWhite = "/sys/class/backlight/mxc_msp430.0/brightness",
            .frontlightDevMixer = "/sys/class/backlight/lm3630a_led/color",
        },
    // NOTE: The Libra apparently suffers from a mysterious issue where completely innocuous
    // WAIT_FOR_UPDATE_COMPLETE ioctls
    //       will mysteriously fail with a timeout (5s)...
    //       This obviously leads to *terrible* user experience, so, until more is understood avout the
    //       issue, bypass this ioctl on this .device. c.f.,
    //       https://github.com/koreader/koreader/issues/7340
    .hasReliableMxcWaitFor = false,
};

// Kobo Nia:
KoboDeviceDescriptor KoboLuna = {
    .device = KoboNia,
    .mark = 7,
    .dpi = 212,
    .canToggleChargingLED = true,
};

// Kobo Elipsa
KoboDeviceDescriptor KoboEuropa = {
    .device = KoboElipsa,
    .mark = 8,
    .dpi = 227,
    .canToggleChargingLED = true,
    .hasGSensor = true,
    .isSunxi = true,
    .batterySysfs = "/sys/class/power_supply/battery",
    .ntxDev = "/dev/input/by-path/platform-ntx_event0-event",
    .touchDev = "/dev/input/by-path/platform-0-0010-event",
};

// Kobo Sage
KoboDeviceDescriptor KoboCadmus = {
    .device = KoboSage,
    .mark = 8,
    .dpi = 300,
    .canToggleChargingLED = true,
    .hasGSensor = true,
    .frontlightSettings =
        {
            .hasNaturalLight = true,
            .hasNaturalLightMixer = true,
            .naturalLightInverted = false,
            .naturalLightMin = 0,
            .naturalLightMax = 10,
            .frontlightDevWhite = "/sys/class/backlight/mxc_msp430.0/brightness",
            .frontlightDevMixer = "/sys/class/leds/aw99703-bl_FL1/color",
        },
    .isSunxi = true,
    .batterySysfs = "/sys/class/power_supply/battery",
    .ntxDev = "/dev/input/by-path/platform-ntx_event0-event",
    .touchDev = "/dev/input/by-path/platform-0-0010-event",

};

// Kobo Libra 2
KoboDeviceDescriptor KoboIo = {
    .device = KoboLibra2,
    .mark = 7,
    .dpi = 300,
    .canToggleChargingLED = true,
    .hasGSensor = true,
    .touchscreenSettings{.invertX = false},
    .frontlightSettings =
        {
            .hasNaturalLight = true,
            .hasNaturalLightMixer = true,
            .naturalLightInverted = true,
            .naturalLightMin = 0,
            .naturalLightMax = 10,
            .frontlightDevWhite = "/sys/class/backlight/mxc_msp430.0/brightness",
            .frontlightDevMixer = "/sys/class/leds/aw99703-bl_FL1/color",
        },
    .batterySysfs = "/sys/class/power_supply/battery",
};

// Kobo Clara 2E
KoboDeviceDescriptor KoboGoldfinch = {
    .device = KoboClara2E,
    .mark = 7,
    .dpi = 300,
    .canToggleChargingLED = true,
    .hasGSensor = true,
    .frontlightSettings =
        {
            .hasNaturalLight = true,
            .hasNaturalLightMixer = true,
            .naturalLightInverted = true,
            .naturalLightMin = 0,
            .naturalLightMax = 10,
            .frontlightDevWhite = "/sys/class/backlight/mxc_msp430.0/brightness",
            .frontlightDevMixer = "/sys/class/leds/aw99703-bl_FL1/color",
        },
    .batterySysfs = "/sys/class/power_supply/battery",
    .powerDev = "/dev/input/by-path/platform-bd71828-pwrkey-event",
};

static QString exec(const char *cmd)
{
    std::array<char, 128> buffer;
    QString result;
    std::unique_ptr<FILE, decltype(&pclose)> pipe(popen(cmd, "r"), pclose);
    if (!pipe)
    {
        throw std::runtime_error("popen() failed!");
    }
    while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr)
    {
        result += buffer.data();
    }
    return result.trimmed();
}

static QRect determineGeometry(const fb_var_screeninfo &vinfo)
{
    int xoff = vinfo.xoffset;
    int yoff = vinfo.yoffset;
    int w = vinfo.xres;
    int h = vinfo.yres;
    return QRect(xoff, yoff, w, h);
}

static QSizeF determinePhysicalSize(const fb_var_screeninfo &vinfo, const QSize &res, int dpi = 300)
{
    int mmWidth = 0, mmHeight = 0;

    if (vinfo.width != 0 && vinfo.height != 0 && vinfo.width != UINT_MAX && vinfo.height != UINT_MAX)
    {
        mmWidth = vinfo.width;
        mmHeight = vinfo.height;
    }
    else
    {
        mmWidth = qRound(res.width() * 25.4 / dpi);
        mmHeight = qRound(res.height() * 25.4 / dpi);
    }

    return QSize(mmWidth, mmHeight);
}

KoboDeviceDescriptor determineDevice()
{
    auto deviceName = exec("/bin/kobo_config.sh 2>/dev/null");
    auto modelNumberStr = exec("cut -f 6 -d ',' /mnt/onboard/.kobo/version | sed -e 's/^[0-]*//'");
    int modelNumber = modelNumberStr.toInt();

    KoboDeviceDescriptor device;
    if (deviceName == "trilogy")
    {
        if (modelNumber == 310)
            device = KoboTrilogyAB;
        else  // if (modelNumber == 320)
            device = KoboTrilogyC;
    }
    else if (deviceName == "pixie")
    {
        device = KoboPixie;
    }
    else if (deviceName == "kraken")
    {
        device = KoboKraken;
    }
    else if (deviceName == "alyssum")
    {
        device = KoboAlyssum;
    }
    else if (deviceName == "pika")
    {
        device = KoboPika;
    }
    else if (deviceName == "phoenix")
    {
        device = KoboPhoenix;
    }
    else if (deviceName == "dragon")
    {
        device = KoboDragon;
    }
    else if (deviceName == "dahlia")
    {
        device = KoboDahlia;
    }
    else if (deviceName == "snow")
    {
        if (modelNumber == 374)
            device = KoboSnow;
        else  // if (modelNumber == 378)
            device = KoboSnowRev2;
    }
    else if (deviceName == "daylight")
    {
        device = KoboDaylight;
    }
    else if (deviceName == "star")
    {
        if (modelNumber == 375)
            device = KoboStar;
        else  // if (modelNumber == 379)
            device = KoboStarRev2;
    }
    else if (deviceName == "nova")
    {
        device = KoboNova;
    }
    else if (deviceName == "frost")
    {
        device = KoboFrost;
    }
    else if (deviceName == "storm")
    {
        device = KoboStorm;
    }
    else if (deviceName == "luna")
    {
        device = KoboLuna;
    }
    else if (deviceName == "europa")
    {
        device = KoboEuropa;
    }
    else if (deviceName == "cadmus")
    {
        device = KoboCadmus;
    }
    else if (deviceName == "io")
    {
        device = KoboIo;
    }
    else if (deviceName == "goldfinch")
    {
        device = KoboGoldfinch;
    }
    else
    {
        device = KoboTrilogyC;
    }

    QString fbDevice = QLatin1String("/dev/fb0");
    if (!QFile::exists(fbDevice))
        fbDevice = QLatin1String("/dev/graphics/fb0");
    if (!QFile::exists(fbDevice))
    {
        qWarning("Unable to figure out framebuffer device. Specify it manually.");
        //            return false;
    }

    int mFbFd = -1;

    if (access(fbDevice.toLatin1().constData(), R_OK | W_OK) == 0)
        mFbFd = QT_OPEN(fbDevice.toLatin1().constData(), O_RDWR);

    if (mFbFd == -1)
    {
        if (access(fbDevice.toLatin1().constData(), R_OK) == 0)
            mFbFd = QT_OPEN(fbDevice.toLatin1().constData(), O_RDONLY);
    }

    // Open the device
    if (mFbFd == -1)
    {
        qErrnoWarning(errno, "Failed to open framebuffer %s", qPrintable(fbDevice));
        //            return false;
    }

    fb_var_screeninfo vinfo;

    if (ioctl(mFbFd, FBIOGET_VSCREENINFO, &vinfo))
    {
        qErrnoWarning(errno, "Error reading variable information");
    }

    QRect geometry = determineGeometry(vinfo);

    auto mPhysicalSize = determinePhysicalSize(vinfo, geometry.size(), device.dpi);

    device.width = geometry.width();
    device.height = geometry.height();
    device.physicalWidth = mPhysicalSize.width();
    device.physicalHeight = mPhysicalSize.height();

    device.modelName = deviceName;
    device.modelNumber = modelNumber;

    close(mFbFd);

    return device;
}
