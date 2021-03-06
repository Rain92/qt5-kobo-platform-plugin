#include "koboplatformadditions.h"

#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include <QCoreApplication>
#include <QDir>
#include <QTextStream>
#include <cmath>
#include <string>

QString str_from_file(const QString& fileName)
{
    QString result;
    QFile file(fileName);
    if (file.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        QTextStream in(&file);
        result = in.readLine().trimmed();
    }
    return result;
}

int int_from_file(const QString& fileName)
{
    return str_from_file(fileName).toInt();
}

static void write_light_value(const std::string& file, int value)
{
    QFile f(file.c_str());
    if (f.open(QIODevice::WriteOnly))
    {
        f.write(QString::number(value).toLatin1());
        f.flush();
        f.close();
    }
}

static void set_light_value(const std::string& dir, int value)
{
    write_light_value(dir + "/bl_power", value > 0 ? 31 : 0);
    write_light_value(dir + "/brightness", value);
}

static bool ntx_io_write(int port, int value)
{
    int fd = -1, retval = -1;
    if ((fd = ::open("/dev/ntx_io", O_RDWR)) != -1)
    {
        retval = ioctl(fd, port, value);
        close(fd);
    }
    return retval != -1;
}

static int ntx_io_read(int port)
{
    int fd = -1, retval = -1, output;
    if ((fd = ::open("/dev/ntx_io", O_RDWR)) != -1)
    {
        retval = ioctl(fd, port, &output);
        close(fd);
    }

    if (retval == -1)
        return -1;

    return output;
}

KoboPlatformAdditions::KoboPlatformAdditions(QObject* parent, const KoboDeviceDescriptor& device)
    : QObject(parent), device(device)
{
}

int KoboPlatformAdditions::getBatteryLevel() const
{
    return int_from_file("/sys/devices/platform/pmic_battery.1/power_supply/mc13892_bat/capacity");
}

bool KoboPlatformAdditions::isBatteryCharging() const
{
    return str_from_file("/sys/devices/platform/pmic_battery.1/power_supply/mc13892_bat/status") ==
           "Charging";
}

bool KoboPlatformAdditions::isUsbConnected() const
{
    return ntx_io_read(108) == 1;
}

void KoboPlatformAdditions::setStatusLedEnabled(bool enabled)
{
    ntx_io_write(101, enabled ? 1 : 0);
}

void KoboPlatformAdditions::setFrontlightLevel(int val, int temp)
{
    val = qMax(0, qMin(val, device.frontlightMaxLevel));
    temp = qMax(0, qMin(temp, device.frontlightMaxTemp));
    if (device.hasComfortLight)
    {
        setNaturalBrightness(val, temp);
    }
    else
    {
        ntx_io_write(241, val);
    }
}

void KoboPlatformAdditions::setNaturalBrightness(int brig, int temp)
{
    const char *fWhite = NULL, *fRed = NULL, *fGreen = NULL, *fMixer = NULL;

    switch (device.device)
    {
        case KoboAuraOne:
            fWhite = "/sys/class/backlight/lm3630a_led1b";
            fRed = "/sys/class/backlight/lm3630a_led1a";
            fGreen = "/sys/class/backlight/lm3630a_ledb";
            break;
        case KoboAuraH2O2_v1:
            fWhite = "/sys/class/backlight/lm3630a_ledb";
            fRed = "/sys/class/backlight/lm3630a_led";
            fGreen = "/sys/class/backlight/lm3630a_leda";
            break;
        case KoboAuraH2O2_v2:
            fWhite = "/sys/class/backlight/lm3630a_ledb";
            fRed = "/sys/class/backlight/lm3630a_leda";
            break;
        case KoboClaraHD:
            fWhite = "/sys/class/backlight/mxc_msp430.0/brightness";
            fMixer = "/sys/class/backlight/lm3630a_led/color";
            break;
        case KoboForma:
            fWhite = "/sys/class/backlight/mxc_msp430.0/brightness";
            fMixer = "/sys/class/backlight/tlc5947_bl/color";
            break;
        case KoboLibra:
            fWhite = "/sys/class/backlight/lm3630a_ledb";
            fRed = "/sys/class/backlight/lm3630a_leda";
            break;
        default:
            break;
    }

    if (KoboForma == device.device || KoboClaraHD == device.device || KoboLibra == device.device)
    {
        if (fWhite != NULL)
            write_light_value(fWhite, brig);
        if (fMixer != NULL)
            write_light_value(fMixer, 10 - temp);
    }
    else
    {
        static const int white_gain = 25;
        static const int red_gain = 24;
        static const int green_gain = 24;
        static const int white_offset = -25;
        static const int red_offset = 0;
        static const int green_offset = -65;
        static const double exponent = 0.25;
        double red = 0.0, green = 0.0, white = 0.0;
        if (brig > 0)
        {
            white =
                std::min(white_gain * pow(brig, exponent) * pow(device.frontlightMaxTemp - temp, exponent) +
                             white_offset,
                         255.0);
        }
        if (temp > 0)
        {
            red = std::min(red_gain * pow(brig, exponent) * pow(temp, exponent) + red_offset, 255.0);
            green = std::min(green_gain * pow(brig, exponent) * pow(temp, exponent) + green_offset, 255.0);
        }
        white = std::max(white, 0.0);
        red = std::max(red, 0.0);
        green = std::max(green, 0.0);

        if (fWhite != NULL)
            set_light_value(fWhite, floor(white));
        if (fRed != NULL)
            set_light_value(fRed, floor(red));
        if (fGreen != NULL)
            set_light_value(fGreen, floor(green));
    }
}
