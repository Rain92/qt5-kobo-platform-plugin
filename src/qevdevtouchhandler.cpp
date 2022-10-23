/****************************************************************************
**
** Copyright (C) 2019 The Qt Company Ltd.
** Copyright (C) 2016 Jolla Ltd, author: <gunnar.sletta@jollamobile.com>
** Contact: https://www.qt.io/licensing/
**
** This file is part of the plugins module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl-3.0.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or (at your option) the GNU General
** Public license version 3 or any later version approved by the KDE Free
** Qt Foundation. The licenses are as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-2.0.html and
** https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qevdevtouchhandler.h"

#include <linux/input.h>
#include <math.h>

#include <QGuiApplication>
#include <QHash>
#include <QLoggingCategory>
#include <QSocketNotifier>
#include <QStringList>
#include <QTouchDevice>
#include <mutex>

#include "qevdevtouchdata.h"
#include "qevdevtouchdata2.h"

QT_BEGIN_NAMESPACE

Q_LOGGING_CATEGORY(qLcEvdevTouch2, "qt.qpa.input")

#define LONG_BITS (sizeof(long) << 3)
#define NUM_LONGS(bits) (((bits) + LONG_BITS - 1) / LONG_BITS)

static inline bool testBit(long bit, const long *array)
{
    return (array[bit / LONG_BITS] >> bit % LONG_BITS) & 1;
}

QEvdevTouchScreenHandler::QEvdevTouchScreenHandler(const QString &device, const QString &spec,
                                                   QObject *parent, KoboFbScreen *koboFbScreen)
    : QObject(parent), m_notify(nullptr), m_fd(-1), d(nullptr), m_device(nullptr)
{
    setObjectName(QLatin1String("Evdev Touch Handler"));

    qCDebug(qLcEvdevTouch2) << "spec: " <<spec;

    const QStringList args = spec.split(QLatin1Char(':'));
    bool experimentaltochdhandler = false;
    bool swapxy = false;
    bool invertx = false;
    bool inverty = false;
    int hw_range_x_max_overwrite = 0;
    int hw_range_y_max_overwrite = 0;
    QRect screenRect;
    int screenrotation = 0;
    for (int i = 0; i < args.count(); ++i)
    {
        if (args.at(i).startsWith(QLatin1String("screenwidth")))
        {
            QString sArg = args.at(i).section(QLatin1Char('='), 1, 1);
            bool ok;
            uint argValue = sArg.toUInt(&ok);
            if (ok)
            {
                screenRect.setWidth(argValue);
            }
        }
        else if (args.at(i).startsWith(QLatin1String("screenheight")))
        {
            QString sArg = args.at(i).section(QLatin1Char('='), 1, 1);
            bool ok;
            uint argValue = sArg.toUInt(&ok);
            if (ok)
            {
                screenRect.setHeight(argValue);
            }
        }
        else if (args.at(i).startsWith(QLatin1String("screenrotation")))
        {
            QString sArg = args.at(i).section(QLatin1Char('='), 1, 1);
            bool ok;
            uint argValue = sArg.toUInt(&ok);
            if (ok)
            {
                screenrotation = argValue;
            }
        }
        else if (args.at(i).startsWith(QLatin1String("hw_range_x_max")))
        {
            QString sArg = args.at(i).section(QLatin1Char('='), 1, 1);
            bool ok;
            uint argValue = sArg.toUInt(&ok);
            if (ok)
            {
                hw_range_x_max_overwrite = argValue;
            }
        }
        else if (args.at(i).startsWith(QLatin1String("hw_range_y_max")))
        {
            QString sArg = args.at(i).section(QLatin1Char('='), 1, 1);
            bool ok;
            uint argValue = sArg.toUInt(&ok);
            if (ok)
            {
                hw_range_y_max_overwrite = argValue;
            }
        }
        else if (args.at(i) == QLatin1String("swapxy"))
        {
            swapxy = true;
        }
        else if (args.at(i) == QLatin1String("invertx"))
        {
            invertx = true;
        }
        else if (args.at(i) == QLatin1String("inverty"))
        {
            inverty = true;
        }
        else if (args.at(i).toLower() == QLatin1String("experimentaltouchhandler"))
        {
            experimentaltochdhandler = true;
        }
    }

    qCDebug(qLcEvdevTouch2, "evdevtouch: Using device %ls", qUtf16Printable(device));

    m_fd = QT_OPEN(device.toLocal8Bit().constData(), O_RDONLY | O_NDELAY, 0);

    if (m_fd >= 0)
    {
        m_notify = new QSocketNotifier(m_fd, QSocketNotifier::Read, this);
        connect(m_notify, &QSocketNotifier::activated, this, &QEvdevTouchScreenHandler::readData);
    }
    else
    {
        qErrnoWarning("evdevtouch: Cannot open input device %ls", qUtf16Printable(device));
        return;
    }


    if(!experimentaltochdhandler)
        d = new QEvdevTouchScreenData(this, args);
    else
        d = new QEvdevTouchScreenData2(this, args, koboFbScreen);

    d->setScreenGeometry(screenRect);
    qCDebug(qLcEvdevTouch2, "evdevtouch: setting screen rect %d %d", screenRect.width(), screenRect.height());

    long absbits[NUM_LONGS(ABS_CNT)];
    if (ioctl(m_fd, EVIOCGBIT(EV_ABS, sizeof(absbits)), absbits) >= 0)
    {
        d->m_typeB = testBit(ABS_MT_SLOT, absbits);
        d->m_singleTouch = !testBit(ABS_MT_POSITION_X, absbits);
    }
    long keybits[NUM_LONGS(KEY_CNT)];
    if (ioctl(m_fd, EVIOCGBIT(EV_KEY, sizeof(keybits)), keybits) >= 0)
        d->m_btnTool = testBit(BTN_TOOL_FINGER, keybits);

    d->deviceNode = device;
    qCDebug(qLcEvdevTouch2, "evdevtouch: %ls: Protocol type %c (%s), filtered=%s",
            qUtf16Printable(d->deviceNode), d->m_typeB ? 'B' : 'A', d->m_singleTouch ? "single" : "multi",
            d->m_filtered ? "yes" : "no");
    if (d->m_filtered)
        qCDebug(qLcEvdevTouch2, " - prediction=%d", d->m_prediction);

    input_absinfo absInfo;
    memset(&absInfo, 0, sizeof(input_absinfo));
    bool has_x_range = false, has_y_range = false;

    if (ioctl(m_fd, EVIOCGABS((d->m_singleTouch ? ABS_X : ABS_MT_POSITION_X)), &absInfo) >= 0)
    {
        qCDebug(qLcEvdevTouch2, "evdevtouch: %ls: min X: %d max X: %d", qUtf16Printable(device),
                absInfo.minimum, absInfo.maximum);
        d->hw_range_x_min = absInfo.minimum;
        d->hw_range_x_max = hw_range_x_max_overwrite > 0 ? hw_range_x_max_overwrite : absInfo.maximum;
        qCDebug(qLcEvdevTouch2, "evdevtouch: overwriting touch x max: %d", hw_range_x_max_overwrite);
        has_x_range = true;
    }

    if (ioctl(m_fd, EVIOCGABS((d->m_singleTouch ? ABS_Y : ABS_MT_POSITION_Y)), &absInfo) >= 0)
    {
        qCDebug(qLcEvdevTouch2, "evdevtouch: %ls: min Y: %d max Y: %d", qUtf16Printable(device),
                absInfo.minimum, absInfo.maximum);
        d->hw_range_y_min = absInfo.minimum;
        d->hw_range_y_max = hw_range_y_max_overwrite > 0 ? hw_range_y_max_overwrite : absInfo.maximum;
        qCDebug(qLcEvdevTouch2, "evdevtouch: overwriting touch y max: %d", hw_range_y_max_overwrite);
        has_y_range = true;
    }

    if (!has_x_range || !has_y_range)
        qWarning("evdevtouch: %ls: Invalid ABS limits, behavior unspecified", qUtf16Printable(device));

    if (ioctl(m_fd, EVIOCGABS(ABS_PRESSURE), &absInfo) >= 0)
    {
        qCDebug(qLcEvdevTouch2, "evdevtouch: %ls: min pressure: %d max pressure: %d", qUtf16Printable(device),
                absInfo.minimum, absInfo.maximum);
        if (absInfo.maximum > absInfo.minimum)
        {
            d->hw_pressure_min = absInfo.minimum;
            d->hw_pressure_max = absInfo.maximum;
        }
    }

    d->m_swapXY = swapxy;
    d->m_invertX = invertx;
    d->m_invertY = inverty;
    if (screenrotation % 90 == 0)
        d->m_rotate = (screenrotation + 3600) % 360;
    else
        d->m_rotate = 0;

    char name[1024];
    if (ioctl(m_fd, EVIOCGNAME(sizeof(name) - 1), name) >= 0)
    {
        d->hw_name = QString::fromLocal8Bit(name);
        qCDebug(qLcEvdevTouch2, "evdevtouch: %ls: device name: %s", qUtf16Printable(device), name);
    }

    bool grabSuccess = !ioctl(m_fd, EVIOCGRAB, (void *)1);
    if (grabSuccess)
        ioctl(m_fd, EVIOCGRAB, (void *)0);
    else
        qWarning("evdevtouch: The device is grabbed by another process. No events will be read.");

    registerTouchDevice();
}

QEvdevTouchScreenHandler::~QEvdevTouchScreenHandler()
{
    if (m_fd >= 0)
        QT_CLOSE(m_fd);

    delete d;

    unregisterTouchDevice();
}

bool QEvdevTouchScreenHandler::isFiltered() const
{
    return d && d->m_filtered;
}

QTouchDevice *QEvdevTouchScreenHandler::touchDevice() const
{
    return m_device;
}

void QEvdevTouchScreenHandler::readData()
{
    ::input_event buffer[32];
    int events = 0;

    int n = 0;
    for (;;)
    {
        events = QT_READ(m_fd, reinterpret_cast<char *>(buffer) + n, sizeof(buffer) - n);
        if (events <= 0)
            goto err;
        n += events;
        if (n % sizeof(::input_event) == 0)
            break;
    }

    n /= sizeof(::input_event);

    for (int i = 0; i < n; ++i)
        d->processInputEvent(&buffer[i]);

    return;

err:
    if (!events)
    {
        qWarning("evdevtouch: Got EOF from input device");
        return;
    }
    else if (events < 0)
    {
        if (errno != EINTR && errno != EAGAIN)
        {
            qErrnoWarning("evdevtouch: Could not read from input device");
            if (errno == ENODEV)
            {  // device got disconnected -> stop reading
                delete m_notify;
                m_notify = nullptr;

                QT_CLOSE(m_fd);
                m_fd = -1;

                unregisterTouchDevice();
            }
            return;
        }
    }
}

void QEvdevTouchScreenHandler::registerTouchDevice()
{
    if (m_device)
        return;

    m_device = new QTouchDevice;
    m_device->setName(d->hw_name);
    m_device->setType(QTouchDevice::TouchScreen);
    m_device->setCapabilities(QTouchDevice::Position | QTouchDevice::Area);
    if (d->hw_pressure_max > d->hw_pressure_min)
        m_device->setCapabilities(m_device->capabilities() | QTouchDevice::Pressure);

    QWindowSystemInterface::registerTouchDevice(m_device);
}

void QEvdevTouchScreenHandler::unregisterTouchDevice()
{
    if (!m_device)
        return;

    // At app exit the cleanup may have already been done, avoid
    // double delete by checking the list first.
    if (QWindowSystemInterface::isTouchDeviceRegistered(m_device))
    {
        QWindowSystemInterface::unregisterTouchDevice(m_device);
        delete m_device;
    }

    m_device = nullptr;
}

QT_END_NAMESPACE
