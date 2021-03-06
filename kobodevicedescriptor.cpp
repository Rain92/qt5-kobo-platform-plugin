#include "kobodevicedescriptor.h"

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

KoboDeviceDescriptor determineDevice()
{
    auto deviceName = exec("/bin/kobo_config.sh 2>/dev/null");
    auto modelNumberStr = exec("cut -f 6 -d ',' /mnt/onboard/.kobo/version | sed -e 's/^[0-]*//'");
    int modelNumber = modelNumberStr.toInt();

    KoboDevice device = KoboOther;
    int dpi = 260;
    int rotation = 90;
    bool invx = false, invy = false;

    int class1 = 90;
    int class2 = 90;

    if (deviceName == "alyssum")
    {
        device = KoboGloHD;
        dpi = 300;
        rotation = 90;
    }
    else if (deviceName == "dahlia")
    {
        device = KoboAuraH2O;
        dpi = 265;
        rotation = 90;
    }
    else if (deviceName == "dragon")
    {
        device = KoboAuraHD;
        dpi = 265;
        rotation = class2;
    }
    else if (deviceName == "phoenix")
    {
        device = KoboAura;
        dpi = 212;
        rotation = class2;
    }
    else if (deviceName == "kraken")
    {
        device = KoboGlo;
        dpi = 213;
        rotation = class2;
    }
    else if (deviceName == "trilogy")
    {
        device = KoboTouch;
        dpi = 167;
        rotation = class2;
    }
    else if (deviceName == "pixie")
    {
        device = KoboMini;
        dpi = 200;
        rotation = class2;
    }
    else if (deviceName == "pika")
    {
        device = KoboTouch2;
        dpi = 167;
        rotation = 90;
    }
    else if (deviceName == "daylight")
    {
        device = KoboAuraOne;
        dpi = 300;
        rotation = 90;
    }
    else if (deviceName == "star")
    {
        device = KoboAura2;
        dpi = 212;
        rotation = 90;
    }
    else if (deviceName == "nova")
    {
        device = KoboClaraHD;
        dpi = 300;
        rotation = 270;
    }
    else if (deviceName == "frost")
    {
        device = KoboForma;
        dpi = 300;
        rotation = 270;
    }
    else if (deviceName == "storm")
    {
        device = KoboLibra;
        dpi = 300;
        rotation = 90;
    }

    else if (deviceName == "snow")
    {
        if (modelNumber == 374)
            device = KoboAuraH2O2_v1;
        else if (modelNumber == 378)
            device = KoboAuraH2O2_v2;

        rotation = class1;
        dpi = 265;
    }

    bool hasComfortlight = device == KoboAuraOne || device == KoboAuraH2O2_v1 || device == KoboAuraH2O2_v2 ||
                           device == KoboClaraHD || device == KoboForma || device == KoboLibra;

    int frontlightMaxLevel = 100;
    int frontlightMaxTemp =
        hasComfortlight ? (device == KoboClaraHD || device == KoboForma || device == KoboLibra ? 10 : 100)
                        : 0;

    KoboDeviceDescriptor descriptor{
        device,          deviceName,         modelNumber,      dpi, 0, 0, 0, 0, {rotation, invx, invy},
        hasComfortlight, frontlightMaxLevel, frontlightMaxTemp};

    return descriptor;
}
