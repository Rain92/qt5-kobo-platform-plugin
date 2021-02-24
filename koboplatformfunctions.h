#ifndef KOBOPLATFORMFUNCTIONS_H
#define KOBOPLATFORMFUNCTIONS_H

#include <QRect>
#include <QtCore/QByteArray>
#include <QtGui/QGuiApplication>

#include "kobodevicedescriptor.h"
#include "refreshmode.h"

class KoboPlatformFunctions
{
public:
    typedef int (*getBatteryLevelType)();
    static QByteArray getBatteryLevelIdentifier() { return QByteArrayLiteral("getBatteryLevel"); }

    static int getBatteryLevel()
    {
        auto func = reinterpret_cast<getBatteryLevelType>(
            QGuiApplication::platformFunction(getBatteryLevelIdentifier()));
        if (func)
            return func();

        return 0;
    }

    typedef bool (*isBatteryChargingType)();
    static QByteArray isBatteryChargingIdentifier() { return QByteArrayLiteral("isBatteryCharging"); }

    static bool isBatteryCharging()
    {
        auto func = reinterpret_cast<isBatteryChargingType>(
            QGuiApplication::platformFunction(isBatteryChargingIdentifier()));
        if (func)
            return func();

        return false;
    }

    typedef void (*setFrontlightLevelType)(int val, int temp);
    static QByteArray setFrontlightLevelIdentifier() { return QByteArrayLiteral("setFrontlightLevel"); }

    static void setFrontlightLevel(int val, int temp)
    {
        auto func = reinterpret_cast<setFrontlightLevelType>(
            QGuiApplication::platformFunction(setFrontlightLevelIdentifier()));
        if (func)
            func(val, temp);
    }

    typedef void (*setPartialRefreshModeType)(PartialRefreshMode partialRefreshMode);
    static QByteArray setPartialRefreshModeIdentifier() { return QByteArrayLiteral("setPartialRefreshMode"); }

    static void setPartialRefreshMode(PartialRefreshMode partialRefreshMode)
    {
        auto func = reinterpret_cast<setPartialRefreshModeType>(
            QGuiApplication::platformFunction(setPartialRefreshModeIdentifier()));
        if (func)
            func(partialRefreshMode);
    }

    typedef void (*setFullScreenRefreshModeType)(WaveForm waveform);
    static QByteArray setFullScreenRefreshModeIdentifier()
    {
        return QByteArrayLiteral("setFullScreenRefreshMode");
    }

    static void setFullScreenRefreshMode(WaveForm waveform)
    {
        auto func = reinterpret_cast<setFullScreenRefreshModeType>(
            QGuiApplication::platformFunction(setFullScreenRefreshModeIdentifier()));
        if (func)
            func(waveform);
    }

    typedef void (*clearScreenType)(bool waitForCompleted);
    static QByteArray clearScreenIdentifier() { return QByteArrayLiteral("clearScreen"); }

    static void clearScreen(bool waitForCompleted)
    {
        auto func =
            reinterpret_cast<clearScreenType>(QGuiApplication::platformFunction(clearScreenIdentifier()));
        if (func)
            func(waitForCompleted);
    }

    typedef void (*enableDitheringType)(bool dithering);
    static QByteArray enableDitheringIdentifier() { return QByteArrayLiteral("enableDithering"); }

    static void enableDithering(bool dithering)
    {
        auto func = reinterpret_cast<enableDitheringType>(
            QGuiApplication::platformFunction(enableDitheringIdentifier()));
        if (func)
            func(dithering);
    }

    typedef void (*doManualRefreshType)(QRect region);
    static QByteArray doManualRefreshIdentifier() { return QByteArrayLiteral("doManualRefresh"); }

    static void doManualRefresh(QRect region)
    {
        auto func = reinterpret_cast<doManualRefreshType>(
            QGuiApplication::platformFunction(doManualRefreshIdentifier()));
        if (func)
            func(region);
    }

    typedef KoboDeviceDescriptor (*getKoboDeviceDescriptorType)();
    static QByteArray getKoboDeviceDescriptorIdentifier()
    {
        return QByteArrayLiteral("getKoboDeviceDescriptor");
    }

    static KoboDeviceDescriptor getKoboDeviceDescriptor()
    {
        auto func = reinterpret_cast<getKoboDeviceDescriptorType>(
            QGuiApplication::platformFunction(getKoboDeviceDescriptorIdentifier()));
        if (func)
            return func();

        return KoboDeviceDescriptor();
    }

    typedef bool (*testInternetConnectionType)(int timeout);
    static QByteArray testInternetConnectionIdentifier()
    {
        return QByteArrayLiteral("testInternetConnection");
    }

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
