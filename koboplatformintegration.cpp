
#include "koboplatformintegration.h"

#include <QtEventDispatcherSupport/private/qgenericunixeventdispatcher_p.h>
#include <QtFbSupport/private/qfbbackingstore_p.h>
#include <QtFbSupport/private/qfbvthandler_p.h>
#include <QtFbSupport/private/qfbwindow_p.h>
#include <QtFontDatabaseSupport/private/qgenericunixfontdatabase_p.h>
#include <QtGui/private/qguiapplication_p.h>
#include <QtInputSupport/private/qevdevtouchmanager_p.h>
#include <QtServiceSupport/private/qgenericunixservices_p.h>
#include <qpa/qplatforminputcontextfactory_p.h>
#include <qpa/qwindowsysteminterface.h>

#include <QtCore/QRegularExpression>

#include "kobobuttonintegration.h"
#include "kobofbscreen.h"
#include "koboplatformfunctions.h"

KoboPlatformIntegration::KoboPlatformIntegration(const QStringList &paramList)
    : m_paramList(paramList),
      m_primaryScreen(nullptr),
      m_fontDb(new QGenericUnixFontDatabase),
      m_services(new QGenericUnixServices),
      m_kbdMgr(nullptr),
      koboKeyboard(nullptr),
      koboAdditions(nullptr),
      debug(false)
{
    if (!m_primaryScreen)
        m_primaryScreen = new KoboFbScreen(paramList);
}

KoboPlatformIntegration::~KoboPlatformIntegration()
{
    QWindowSystemInterface::handleScreenRemoved(m_primaryScreen);
}

void KoboPlatformIntegration::initialize()
{
    if (m_primaryScreen->initialize())
        QWindowSystemInterface::handleScreenAdded(m_primaryScreen);
    else
        qWarning("kobofb: Failed to initialize screen");

    m_inputContext = QPlatformInputContextFactory::create();

    m_vtHandler.reset(new QFbVtHandler);

    createInputHandlers();
}

bool KoboPlatformIntegration::hasCapability(QPlatformIntegration::Capability cap) const
{
    switch (cap)
    {
        case ThreadedPixmaps:
            return true;
        case WindowManagement:
            return false;
        default:
            return QPlatformIntegration::hasCapability(cap);
    }
}

QPlatformBackingStore *KoboPlatformIntegration::createPlatformBackingStore(QWindow *window) const
{
    return new QFbBackingStore(window);
}

QPlatformWindow *KoboPlatformIntegration::createPlatformWindow(QWindow *window) const
{
    return new QFbWindow(window);
}

QAbstractEventDispatcher *KoboPlatformIntegration::createEventDispatcher() const
{
    return createUnixEventDispatcher();
}

QList<QPlatformScreen *> KoboPlatformIntegration::screens() const
{
    QList<QPlatformScreen *> list;
    list.append(m_primaryScreen);
    return list;
}

QPlatformFontDatabase *KoboPlatformIntegration::fontDatabase() const
{
    return m_fontDb.data();
}

QPlatformServices *KoboPlatformIntegration::services() const
{
    return m_services.data();
}

void KoboPlatformIntegration::createInputHandlers()
{
    int touchscreenRotation = 0;
    QString touchscreenDevice("/dev/input/event1");
    QRegularExpression rotTouchRx("touchscreen_rotate=(.*)");
    QRegularExpression touchDevRx("touchscreen_device=(.*)");

    for (const QString &arg : qAsConst(m_paramList))
    {
        if (arg.contains("debug"))
            debug = true;

        QRegularExpressionMatch match;
        if (arg.contains(rotTouchRx, &match))
            touchscreenRotation = match.captured(1).toInt();
        if (arg.contains(touchDevRx, &match))
            touchscreenDevice = match.captured(1);
    }

    QString evdevTouchArgs(QString("%1:rotate=%2").arg(touchscreenDevice).arg(touchscreenRotation));

    new QEvdevTouchManager(QLatin1String("EvdevTouch"), evdevTouchArgs, this);

    koboKeyboard = new KoboButtonIntegration(this, debug);
    koboAdditions = new KoboPlatformAdditions(this, *this->m_primaryScreen->deviceDescriptor());
}

QPlatformNativeInterface *KoboPlatformIntegration::nativeInterface() const
{
    return const_cast<KoboPlatformIntegration *>(this);
}

QFunctionPointer KoboPlatformIntegration::platformFunction(const QByteArray &function) const
{
    if (function == KoboPlatformFunctions::setFrontlightLevelIdentifier())
        return QFunctionPointer(setFrontlightLevelStatic);
    else if (function == KoboPlatformFunctions::getBatteryLevelIdentifier())
        return QFunctionPointer(getBatteryLevelStatic);
    else if (function == KoboPlatformFunctions::isBatteryChargingIdentifier())
        return QFunctionPointer(isBatteryChargingStatic);
    else if (function == KoboPlatformFunctions::setPartialRefreshModeIdentifier())
        return QFunctionPointer(setPartialRefreshModeStatic);
    else if (function == KoboPlatformFunctions::doManualRefreshIdentifier())
        return QFunctionPointer(doManualRefreshStatic);
    else if (function == KoboPlatformFunctions::getKoboDeviceDescriptorIdentifier())
        return QFunctionPointer(getKoboDeviceDescriptorStatic);

    else if (function == KoboPlatformFunctions::testInternetConnectionIdentifier())
        return QFunctionPointer(KoboWifiManager::testInternetConnection);
    else if (function == KoboPlatformFunctions::enableWiFiConnectionIdentifier())
        return QFunctionPointer(KoboWifiManager::enableWiFiConnection);
    else if (function == KoboPlatformFunctions::disableWiFiConnectionIdentifier())
        return QFunctionPointer(KoboWifiManager::disableWiFiConnection);

    return 0;
}

void KoboPlatformIntegration::setFrontlightLevelStatic(int val, int temp)
{
    KoboPlatformIntegration *self =
        static_cast<KoboPlatformIntegration *>(QGuiApplicationPrivate::platformIntegration());

    self->koboAdditions->setFrontlightLevel(val, temp);
}

int KoboPlatformIntegration::getBatteryLevelStatic()
{
    KoboPlatformIntegration *self =
        static_cast<KoboPlatformIntegration *>(QGuiApplicationPrivate::platformIntegration());

    return self->koboAdditions->getBatteryLevel();
}

bool KoboPlatformIntegration::isBatteryChargingStatic()
{
    KoboPlatformIntegration *self =
        static_cast<KoboPlatformIntegration *>(QGuiApplicationPrivate::platformIntegration());

    return self->koboAdditions->isBatteryCharging();
}

void KoboPlatformIntegration::setPartialRefreshModeStatic(PartialRefreshMode partial_refresh_mode)
{
    KoboPlatformIntegration *self =
        static_cast<KoboPlatformIntegration *>(QGuiApplicationPrivate::platformIntegration());

    self->m_primaryScreen->setPartialRefreshMode(partial_refresh_mode);
}

void KoboPlatformIntegration::doManualRefreshStatic(QRect region)
{
    KoboPlatformIntegration *self =
        static_cast<KoboPlatformIntegration *>(QGuiApplicationPrivate::platformIntegration());

    self->m_primaryScreen->doManualRefresh(region);
}

KoboDeviceDescriptor KoboPlatformIntegration::getKoboDeviceDescriptorStatic()
{
    KoboPlatformIntegration *self =
        static_cast<KoboPlatformIntegration *>(QGuiApplicationPrivate::platformIntegration());

    return *self->m_primaryScreen->deviceDescriptor();
}
