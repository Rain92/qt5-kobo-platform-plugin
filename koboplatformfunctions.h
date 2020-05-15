#ifndef KOBOPLATFORMFUNCTIONS_H
#define KOBOPLATFORMFUNCTIONS_H

#include <QRect>
#include <QtCore/QByteArray>
#include <QtGui/QGuiApplication>

#include "kobodevicedescriptor.h"
#include "partialrefreshmode.h"

class KoboPlatformFunctions
{
public:
    typedef int (*getBatteryLevelType)();
    static QByteArray getBatteryLevelIdentifier() { return QByteArrayLiteral("getBatteryLevel"); }

    static int getBatteryLevel()
    {
        auto func =
            reinterpret_cast<getBatteryLevelType>(QGuiApplication::platformFunction(getBatteryLevelIdentifier()));
        if (func)
            return func();

        return 0;
    }

    typedef bool (*isBatteryChargingType)();
    static QByteArray isBatteryChargingIdentifier() { return QByteArrayLiteral("isBatteryCharging"); }

    static bool isBatteryCharging()
    {
        auto func =
            reinterpret_cast<isBatteryChargingType>(QGuiApplication::platformFunction(isBatteryChargingIdentifier()));
        if (func)
            return func();

        return false;
    }

    typedef void (*setFrontlightLevelType)(int val, int temp);
    static QByteArray setFrontlightLevelIdentifier() { return QByteArrayLiteral("setFrontlightLevel"); }

    static void setFrontlightLevel(int val, int temp)
    {
        auto func =
            reinterpret_cast<setFrontlightLevelType>(QGuiApplication::platformFunction(setFrontlightLevelIdentifier()));
        if (func)
            func(val, temp);
    }

    typedef void (*setPartialRefreshModeType)(PartialRefreshMode partial_refresh_mode);
    static QByteArray setPartialRefreshModeIdentifier() { return QByteArrayLiteral("setPartialRefreshMode"); }

    static void setPartialRefreshMode(PartialRefreshMode partial_refresh_mode)
    {
        auto func = reinterpret_cast<setPartialRefreshModeType>(
            QGuiApplication::platformFunction(setPartialRefreshModeIdentifier()));
        if (func)
            func(partial_refresh_mode);
    }

    typedef void (*doManualRefreshType)(QRect region);
    static QByteArray doManualRefreshIdentifier() { return QByteArrayLiteral("doManualRefresh"); }

    static void doManualRefresh(QRect region)
    {
        auto func =
            reinterpret_cast<doManualRefreshType>(QGuiApplication::platformFunction(doManualRefreshIdentifier()));
        if (func)
            func(region);
    }

    typedef KoboDeviceDescriptor (*getKoboDeviceDescriptorType)();
    static QByteArray getKoboDeviceDescriptorIdentifier() { return QByteArrayLiteral("getKoboDeviceDescriptor"); }

    static KoboDeviceDescriptor getKoboDeviceDescriptor()
    {
        auto func = reinterpret_cast<getKoboDeviceDescriptorType>(
            QGuiApplication::platformFunction(getKoboDeviceDescriptorIdentifier()));
        if (func)
            return func();

        return KoboDeviceDescriptor();
    }

    typedef bool (*testInternetConnectionType)(int timeout);
    static QByteArray testInternetConnectionIdentifier() { return QByteArrayLiteral("testInternetConnection"); }

    static bool testInternetConnection(int timeout = 2)
    {
        auto func = reinterpret_cast<testInternetConnectionType>(
            QGuiApplication::platformFunction(testInternetConnectionIdentifier()));
        if (func)
            return func(timeout);

        return false;
    }

    typedef void (*enableWiFiConnectionType)();
    static QByteArray enableWiFiConnectionIdentifier() { return QByteArrayLiteral("enableWiFiConnection"); }

    static void enableWiFiConnection()
    {
        auto func = reinterpret_cast<enableWiFiConnectionType>(
            QGuiApplication::platformFunction(enableWiFiConnectionIdentifier()));
        if (func)
            func();
    }

    typedef void (*disableWiFiConnectionType)();
    static QByteArray disableWiFiConnectionIdentifier() { return QByteArrayLiteral("disableWiFiConnection"); }

    static void disableWiFiConnection()
    {
        auto func = reinterpret_cast<disableWiFiConnectionType>(
            QGuiApplication::platformFunction(disableWiFiConnectionIdentifier()));
        if (func)
            func();
    }
};

#endif  // KOBOPLATFORMFUNCTIONS_H
