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

#include "qevdevtouchhandlerthread.h"

#include <linux/input.h>
#include <math.h>

#include <QGuiApplication>
#include <QHash>
#include <QLoggingCategory>
#include <QSocketNotifier>
#include <QStringList>
#include <QTouchDevice>
#include <mutex>

QT_BEGIN_NAMESPACE

QEvdevTouchScreenHandlerThread::QEvdevTouchScreenHandlerThread(const QString &device, const QString &spec,
                                                               QObject *parent, KoboFbScreen *koboFbScreen) :
      m_device(device),
      m_spec(spec),
      m_handler(nullptr),
      m_touchDeviceRegistered(false),
      m_touchUpdatePending(false),
      m_filterWindow(nullptr),
      m_touchRate(-1),
    koboFbScreen(koboFbScreen)

{
    start();
}

QEvdevTouchScreenHandlerThread::~QEvdevTouchScreenHandlerThread()
{
    quit();
    wait();
}

void QEvdevTouchScreenHandlerThread::run()
{
    m_handler = new QEvdevTouchScreenHandler(m_device, m_spec, nullptr, koboFbScreen);

    if (m_handler->isFiltered())
        connect(m_handler, &QEvdevTouchScreenHandler::touchPointsUpdated, this,
                &QEvdevTouchScreenHandlerThread::scheduleTouchPointUpdate);

    // Report the registration to the parent thread by invoking the method asynchronously
    QMetaObject::invokeMethod(this, "notifyTouchDeviceRegistered", Qt::QueuedConnection);

    exec();

    delete m_handler;
    m_handler = nullptr;
}

bool QEvdevTouchScreenHandlerThread::isTouchDeviceRegistered() const
{
    return m_touchDeviceRegistered;
}

void QEvdevTouchScreenHandlerThread::notifyTouchDeviceRegistered()
{
    m_touchDeviceRegistered = true;
    emit touchDeviceRegistered();
}

void QEvdevTouchScreenHandlerThread::scheduleTouchPointUpdate()
{
    QWindow *window = QGuiApplication::focusWindow();
    if (window != m_filterWindow)
    {
        if (m_filterWindow)
            m_filterWindow->removeEventFilter(this);
        m_filterWindow = window;
        if (m_filterWindow)
            m_filterWindow->installEventFilter(this);
    }
    if (m_filterWindow)
    {
        m_touchUpdatePending = true;
        m_filterWindow->requestUpdate();
    }
}

bool QEvdevTouchScreenHandlerThread::eventFilter(QObject *object, QEvent *event)
{
    if (m_touchUpdatePending && object == m_filterWindow && event->type() == QEvent::UpdateRequest)
    {
        m_touchUpdatePending = false;
        filterAndSendTouchPoints();
    }
    return false;
}

void QEvdevTouchScreenHandlerThread::filterAndSendTouchPoints()
{
    QRect winRect = m_handler->d->screenGeometry();
    if (winRect.isNull())
        return;

    float vsyncDelta = 1.0f / QGuiApplication::primaryScreen()->refreshRate();

    QHash<int, FilteredTouchPoint> filteredPoints;

    m_handler->d->m_mutex.lock();

    double time = m_handler->d->m_timeStamp;
    double lastTime = m_handler->d->m_lastTimeStamp;
    double touchDelta = time - lastTime;
    if (m_touchRate < 0 || touchDelta > vsyncDelta)
    {
        // We're at the very start, with nothing to go on, so make a guess
        // that the touch rate will be somewhere in the range of half a vsync.
        // This doesn't have to be accurate as we will calibrate it over time,
        // but it gives us a better starting point so calibration will be
        // slightly quicker. If, on the other hand, we already have an
        // estimate, we'll leave it as is and keep it.
        if (m_touchRate < 0)
            m_touchRate = (1.0 / QGuiApplication::primaryScreen()->refreshRate()) / 2.0;
    }
    else
    {
        // Update our estimate for the touch rate. We're making the assumption
        // that this value will be mostly accurate with the occational bump,
        // so we're weighting the existing value high compared to the update.
        const double ratio = 0.9;
        m_touchRate = sqrt(m_touchRate * m_touchRate * ratio + touchDelta * touchDelta * (1.0 - ratio));
    }

    QList<QWindowSystemInterface::TouchPoint> points = m_handler->d->m_touchPoints;
    QList<QWindowSystemInterface::TouchPoint> lastPoints = m_handler->d->m_lastTouchPoints;

    m_handler->d->m_mutex.unlock();

    for (int i = 0; i < points.size(); ++i)
    {
        QWindowSystemInterface::TouchPoint &tp = points[i];
        QPointF pos = tp.normalPosition;
        FilteredTouchPoint f;

        QWindowSystemInterface::TouchPoint ltp;
        ltp.id = -1;
        for (int j = 0; j < lastPoints.size(); ++j)
        {
            if (lastPoints.at(j).id == tp.id)
            {
                ltp = lastPoints.at(j);
                break;
            }
        }

        QPointF velocity;
        if (lastTime != 0 && ltp.id >= 0)
            velocity = (pos - ltp.normalPosition) / m_touchRate;
        if (m_filteredPoints.contains(tp.id))
        {
            f = m_filteredPoints.take(tp.id);
            f.x.update(pos.x(), velocity.x(), vsyncDelta);
            f.y.update(pos.y(), velocity.y(), vsyncDelta);
            pos = QPointF(f.x.position(), f.y.position());
        }
        else
        {
            f.x.initialize(pos.x(), velocity.x());
            f.y.initialize(pos.y(), velocity.y());
            // Make sure the first instance of a touch point we send has the
            // 'pressed' state.
            if (tp.state != Qt::TouchPointPressed)
                tp.state = Qt::TouchPointPressed;
        }

        tp.velocity = QVector2D(f.x.velocity() * winRect.width(), f.y.velocity() * winRect.height());

        qreal filteredNormalizedX = f.x.position() + f.x.velocity() * m_handler->d->m_prediction / 1000.0;
        qreal filteredNormalizedY = f.y.position() + f.y.velocity() * m_handler->d->m_prediction / 1000.0;

        // Clamp to the screen
        tp.normalPosition =
            QPointF(qBound<qreal>(0, filteredNormalizedX, 1), qBound<qreal>(0, filteredNormalizedY, 1));

        qreal x = winRect.x() + (tp.normalPosition.x() * (winRect.width() - 1));
        qreal y = winRect.y() + (tp.normalPosition.y() * (winRect.height() - 1));

        tp.area.moveCenter(QPointF(x, y));

        // Store the touch point for later so we can release it if we've
        // missed the actual release between our last update and this.
        f.touchPoint = tp;

        // Don't store the point for future reference if it is a release.
        if (tp.state != Qt::TouchPointReleased)
            filteredPoints[tp.id] = f;
    }

    for (QHash<int, FilteredTouchPoint>::const_iterator it = m_filteredPoints.constBegin(),
                                                        end = m_filteredPoints.constEnd();
         it != end; ++it)
    {
        const FilteredTouchPoint &f = it.value();
        QWindowSystemInterface::TouchPoint tp = f.touchPoint;
        tp.state = Qt::TouchPointReleased;
        tp.velocity = QVector2D();
        points.append(tp);
    }

    m_filteredPoints = filteredPoints;

    qDebug() << "sending points:" << points.first().normalPosition;

    QWindowSystemInterface::handleTouchEvent(nullptr, m_handler->touchDevice(), points);
}

QT_END_NAMESPACE
