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

#include "qevdevtouchdata.h"

#include <linux/input.h>
#include <math.h>

#include <QGuiApplication>
#include <QHash>
#include <QLoggingCategory>
#include <QSocketNotifier>
#include <QStringList>
#include <QTouchDevice>
#include <mutex>

#include "qevdevtouchhandler.h"

QT_BEGIN_NAMESPACE

Q_LOGGING_CATEGORY(qLcEvdevTouch, "qt.qpa.input")
Q_LOGGING_CATEGORY(qLcEvents, "qt.qpa.input.events")

QPointF QEvdevTouchScreenData::transformTouchPoint(const QPointF &p)
{
    QPointF tp = p;

    if (m_swapXY)
        tp = tp.transposed();

    tp = {m_invertX ? 1 - tp.x() : tp.x(), m_invertY ? 1 - tp.y() : tp.y()};

    if (m_rotate == 90)
        tp = {tp.y(), 1 - tp.x()};
    if (m_rotate == 180)
        tp = {1 - tp.x(), 1 - tp.y()};
    if (m_rotate == 270)
        tp = {1 - tp.y(), tp.x()};

    return tp;
}

QEvdevTouchScreenData::QEvdevTouchScreenData(QEvdevTouchScreenHandler *q_ptr, const QStringList &args)
    : q(q_ptr),
      m_lastEventType(-1),
      m_currentSlot(0),
      m_timeStamp(0),
      m_lastTimeStamp(0),
      hw_range_x_min(0),
      hw_range_x_max(0),
      hw_range_y_min(0),
      hw_range_y_max(0),
      hw_pressure_min(0),
      hw_pressure_max(0),
      m_forceToActiveWindow(false),
      m_typeB(false),
      m_singleTouch(false),
      m_btnTool(false),
      m_filtered(false),
      m_prediction(0)
{
    for (const QString &arg : args)
    {
        if (arg == QStringLiteral("force_window"))
            m_forceToActiveWindow = true;
        else if (arg == QStringLiteral("filtered"))
            m_filtered = true;
        else if (arg.startsWith(QStringLiteral("prediction=")))
            m_prediction = arg.midRef(11).toInt();
    }
}

void QEvdevTouchScreenData::addTouchPoint(const Contact &contact, Qt::TouchPointStates *combinedStates)
{
    if (contact.x == -1 && contact.y == -1)
        return;  // failsafe, for devices with ABS_MT_TRACKING_ID not starting at 0

    QWindowSystemInterface::TouchPoint tp;
    tp.id = contact.trackingId;
    tp.flags = contact.flags;
    tp.state = contact.state;
    *combinedStates |= tp.state;

    // Store the HW coordinates for now, will be updated later.
    tp.area = QRectF(0, 0, contact.maj, contact.maj);
    tp.area.moveCenter(QPoint(contact.x, contact.y));
    tp.pressure = contact.pressure;

    // Get a normalized position in range 0..1.
    tp.normalPosition = QPointF((contact.x - hw_range_x_min) / qreal(hw_range_x_max - hw_range_x_min),
                                (contact.y - hw_range_y_min) / qreal(hw_range_y_max - hw_range_y_min));

    tp.rawPositions.append(QPointF(contact.x, contact.y));

    qCDebug(qLcEvdevTouch) << "Adding touch point:" << tp.id << tp.uniqueId;
    qCDebug(qLcEvdevTouch) << "Touchpoint raw position:" << tp.rawPositions.last();
    qCDebug(qLcEvdevTouch) << "Touchpoint normal position:" << tp.normalPosition;

    tp.normalPosition = transformTouchPoint(tp.normalPosition);

    qCDebug(qLcEvdevTouch) << "Touchpoint transformed normal position:" << tp.normalPosition;

    m_touchPoints.append(tp);
}

void QEvdevTouchScreenData::processInputEvent(input_event *data)
{
    if (data->type == EV_ABS)
    {
        if (data->code == ABS_MT_POSITION_X || (m_singleTouch && data->code == ABS_X))
        {
            qCDebug(qLcEvdevTouch) << "EV_ABS MT_POS_X" << data->value;
            m_currentData.x = qBound(hw_range_x_min, data->value, hw_range_x_max);
            if (m_singleTouch)
            {
                m_contacts[m_currentSlot].x = m_currentData.x;
                qCDebug(qLcEvdevTouch) << "EV_ABS MT_POS_X Single Touch";
            }
            if (m_typeB)
            {
                qCDebug(qLcEvdevTouch) << "EV_ABS MT_POS_X Type B";
                m_contacts[m_currentSlot].x = m_currentData.x;
                if (m_contacts[m_currentSlot].state == Qt::TouchPointStationary)
                    m_contacts[m_currentSlot].state = Qt::TouchPointMoved;
            }
        }
        else if (data->code == ABS_MT_POSITION_Y || (m_singleTouch && data->code == ABS_Y))
        {
            m_currentData.y = qBound(hw_range_y_min, data->value, hw_range_y_max);
            qCDebug(qLcEvdevTouch) << "EV_ABS MT_POS_Y" << data->value;
            if (m_singleTouch)
            {
                m_contacts[m_currentSlot].y = m_currentData.y;
                qCDebug(qLcEvdevTouch) << "EV_ABS MT_POS_Y Single touch";
            }
            if (m_typeB)
            {
                qCDebug(qLcEvdevTouch) << "EV_ABS MT_POS_Y Type B";
                m_contacts[m_currentSlot].y = m_currentData.y;
                if (m_contacts[m_currentSlot].state == Qt::TouchPointStationary)
                    m_contacts[m_currentSlot].state = Qt::TouchPointMoved;
            }
        }
        else if (data->code == ABS_MT_TRACKING_ID)
        {
            m_currentData.trackingId = data->value;

            // QT apparently can't handle high tracking IDs well?
            if (m_currentData.trackingId > 2)
                m_currentData.trackingId = 1;

            qCDebug(qLcEvdevTouch) << "EV_ABS TRACKING_ID " << m_currentData.trackingId;
            if (m_typeB)
            {
                if (m_currentData.trackingId == -1)
                {
                    m_contacts[m_currentSlot].state = Qt::TouchPointReleased;
                    qCDebug(qLcEvdevTouch) << "EV_ABS TRACKING_ID = -1 touch point released";
                }
                else
                {
                    m_contacts[m_currentSlot].state = Qt::TouchPointPressed;
                    m_contacts[m_currentSlot].trackingId = m_currentData.trackingId;
                    qCDebug(qLcEvdevTouch) << "EV_ABS TRACKING_ID != -1 touch point pressed";
                }
            }
        }
        else if (data->code == ABS_MT_TOUCH_MAJOR)
        {
            qCDebug(qLcEvdevTouch) << "EV_ABS TOUCH_MAJOR";
            m_currentData.maj = data->value;
            if (data->value == 0 && !m_btnTool)
                m_currentData.state = Qt::TouchPointReleased;
            if (m_typeB)
                m_contacts[m_currentSlot].maj = m_currentData.maj;
        }
        else if (data->code == ABS_PRESSURE || data->code == ABS_MT_PRESSURE)
        {
            qCDebug(qLcEvdevTouch) << "EV_ABS PRESSURE";
            if (Q_UNLIKELY(qLcEvents().isDebugEnabled()))
                qCDebug(qLcEvents, "EV_ABS code 0x%x: pressure %d; bounding to [%d,%d]", data->code,
                        data->value, hw_pressure_min, hw_pressure_max);
            m_currentData.pressure = qBound(hw_pressure_min, data->value, hw_pressure_max);
            if (m_typeB || m_singleTouch)
                m_contacts[m_currentSlot].pressure = m_currentData.pressure;
        }
        else if (data->code == ABS_MT_SLOT)
        {
            m_currentSlot = data->value;
            qCDebug(qLcEvdevTouch) << "EV_ABS SLOT";
        }
    }
    else if (data->type == EV_KEY && !m_typeB)
    {
        qCDebug(qLcEvdevTouch) << "EV_KEY";
        if (data->code == BTN_TOUCH && data->value == 0)
        {
            m_contacts[m_currentSlot].state = Qt::TouchPointReleased;
            qCDebug(qLcEvdevTouch) << "EV_KEY BTN_TOUCH 0 touchpoint released";
        }

        // On some devices (Glo HD) ABS_MT_TRACKING_ID starts at 1
        // => there will be malformed events with ABS_MT_TRACKING_ID 0 => filter those at addTouchPoint().
        if (data->code == BTN_TOUCH && data->value == 1)
        {
            m_contacts[m_currentSlot].state = Qt::TouchPointPressed;
            qCDebug(qLcEvdevTouch) << "EV_KEY BTN_TOUCH 1 touchpoint pressed";
        }
    }
    else if (data->type == EV_SYN && data->code == SYN_MT_REPORT && m_lastEventType != EV_SYN)
    {
        qCDebug(qLcEvdevTouch) << "EV_SYN MT_REPORT && lastEvent was not EV_SYN";
        // If there is no tracking id, one will be generated later.
        // Until that use a temporary key.
        int key = m_currentData.trackingId;
        if (key == -1)
            key = m_contacts.count();

        m_contacts.insert(key, m_currentData);
        m_currentData = Contact();
    }
    else if (data->type == EV_SYN && data->code == SYN_REPORT)
    {
        qCDebug(qLcEvdevTouch) << "EV_SYN SYN_REPORT";

        // Ensure valid IDs even when the driver does not report ABS_MT_TRACKING_ID.
        if (!m_contacts.isEmpty() && m_contacts.constBegin().value().trackingId == -1)
            assignIds();

        std::unique_lock<QMutex> locker;
        if (m_filtered)
            locker = std::unique_lock<QMutex>{m_mutex};

        // update timestamps
        m_lastTimeStamp = m_timeStamp;
        m_timeStamp = data->time.tv_sec + data->time.tv_usec / 1000000.0;

        m_lastTouchPoints = m_touchPoints;
        m_touchPoints.clear();
        Qt::TouchPointStates combinedStates;
        bool hasPressure = false;

        for (auto i = m_contacts.begin(), end = m_contacts.end(); i != end; /*erasing*/)
        {
            auto it = i++;

            Contact &contact(it.value());

            if (!contact.state)
                continue;

            int key = m_typeB ? it.key() : contact.trackingId;
            if (!m_typeB && m_lastContacts.contains(key))
            {
                const Contact &prev(m_lastContacts.value(key));
                if (contact.state == Qt::TouchPointReleased)
                {
                    // Copy over the previous values for released points, just in case.
                    contact.x = prev.x;
                    contact.y = prev.y;
                    contact.maj = prev.maj;
                }
                else
                {
                    contact.state = (prev.x == contact.x && prev.y == contact.y) ? Qt::TouchPointStationary
                                                                                 : Qt::TouchPointMoved;
                }
            }

            // Avoid reporting a contact in released state more than once.
            if (!m_typeB && contact.state == Qt::TouchPointReleased && !m_lastContacts.contains(key))
            {
                m_contacts.erase(it);
                qCDebug(qLcEvdevTouch) << "Erase contact since touchpoint released";
                continue;
            }

            if (contact.pressure)
                hasPressure = true;

            addTouchPoint(contact, &combinedStates);
        }

        // Now look for contacts that have disappeared since the last sync.
        for (auto it = m_lastContacts.begin(), end = m_lastContacts.end(); it != end; ++it)
        {
            Contact &contact(it.value());
            int key = m_typeB ? it.key() : contact.trackingId;
            if (m_typeB)
            {
                if (contact.trackingId != m_contacts[key].trackingId && contact.state)
                {
                    contact.state = Qt::TouchPointReleased;
                    addTouchPoint(contact, &combinedStates);
                }
            }
            else
            {
                if (!m_contacts.contains(key))
                {
                    contact.state = Qt::TouchPointReleased;
                    addTouchPoint(contact, &combinedStates);
                }
            }
        }

        // Remove contacts that have just been reported as released.
        for (auto i = m_contacts.begin(), end = m_contacts.end(); i != end; /*erasing*/)
        {
            auto it = i++;

            Contact &contact(it.value());

            if (!contact.state)
                continue;

            if (contact.state == Qt::TouchPointReleased)
            {
                if (m_typeB)
                    contact.state = static_cast<Qt::TouchPointState>(0);
                else
                {
                    m_contacts.erase(it);
                    qCDebug(qLcEvdevTouch) << "Contact Erase since state is released";
                }
            }
            else
            {
                contact.state = Qt::TouchPointStationary;
            }
        }

        m_lastContacts = m_contacts;
        if (!m_typeB && !m_singleTouch)
            m_contacts.clear();

        if (!m_touchPoints.isEmpty() && (hasPressure || combinedStates != Qt::TouchPointStationary))
            reportPoints();
    }

    qCDebug(qLcEvdevTouch) << "Contact state:" << m_contacts[0].state;

    m_lastEventType = data->type;
}

int QEvdevTouchScreenData::findClosestContact(const QHash<int, Contact> &contacts, int x, int y, int *dist)
{
    int minDist = -1, id = -1;
    for (QHash<int, Contact>::const_iterator it = contacts.constBegin(), ite = contacts.constEnd(); it != ite;
         ++it)
    {
        const Contact &contact(it.value());
        int dx = x - contact.x;
        int dy = y - contact.y;
        int dist = dx * dx + dy * dy;
        if (minDist == -1 || dist < minDist)
        {
            minDist = dist;
            id = contact.trackingId;
        }
    }
    if (dist)
        *dist = minDist;
    return id;
}

void QEvdevTouchScreenData::assignIds()
{
    QHash<int, Contact> candidates = m_lastContacts, pending = m_contacts, newContacts;
    int maxId = -1;
    QHash<int, Contact>::iterator it, ite, bestMatch;
    while (!pending.isEmpty() && !candidates.isEmpty())
    {
        int bestDist = -1, bestId = 0;
        for (it = pending.begin(), ite = pending.end(); it != ite; ++it)
        {
            int dist;
            int id = findClosestContact(candidates, it->x, it->y, &dist);
            if (id >= 0 && (bestDist == -1 || dist < bestDist))
            {
                bestDist = dist;
                bestId = id;
                bestMatch = it;
            }
        }
        if (bestDist >= 0)
        {
            bestMatch->trackingId = bestId;
            newContacts.insert(bestId, *bestMatch);
            candidates.remove(bestId);
            pending.erase(bestMatch);
            if (bestId > maxId)
                maxId = bestId;
        }
    }
    if (candidates.isEmpty())
    {
        for (it = pending.begin(), ite = pending.end(); it != ite; ++it)
        {
            it->trackingId = ++maxId;
            newContacts.insert(it->trackingId, *it);
        }
    }
    m_contacts = newContacts;
}

QRect QEvdevTouchScreenData::screenGeometry() const
{
    return m_screenGeometry;
}

void QEvdevTouchScreenData::setScreenGeometry(QRect rect)
{
    m_screenGeometry = rect;
}

void QEvdevTouchScreenData::reportPoints()
{
    QRect winRect = screenGeometry();
    if (winRect.isNull())
        return;

    //    const int hw_w = hw_range_x_max - hw_range_x_min;
    //    const int hw_h = hw_range_y_max - hw_range_y_min;

    // Map the coordinates based on the normalized position. QPA expects 'area'
    // to be in screen coordinates.
    const int pointCount = m_touchPoints.count();
    for (int i = 0; i < pointCount; ++i)
    {
        QWindowSystemInterface::TouchPoint &tp(m_touchPoints[i]);

        // Generate a screen position that is always inside the active window
        // or the primary screen.  Even though we report this as a QRectF, internally
        // Qt uses QRect/QPoint so we need to bound the size to winRect.size() - QSize(1, 1)
        const qreal wx = winRect.left() + tp.normalPosition.x() * (winRect.width() - 1);
        const qreal wy = winRect.top() + tp.normalPosition.y() * (winRect.height() - 1);
        //        const qreal sizeRatio = (winRect.width() + winRect.height()) / qreal(hw_w + hw_h);
        //        if (tp.area.width() == -1)  // touch major was not provided
        tp.area = QRectF(0, 0, 8, 8);
        //        else
        //            tp.area = QRectF(0, 0, tp.area.width() * sizeRatio, tp.area.height() * sizeRatio);
        tp.area.moveCenter(QPointF(wx, wy));

        // Calculate normalized pressure.
        if (!hw_pressure_min && !hw_pressure_max)
            tp.pressure = tp.state == Qt::TouchPointReleased ? 0 : 1;
        else
            tp.pressure = (tp.pressure - hw_pressure_min) / qreal(hw_pressure_max - hw_pressure_min);

        if (Q_UNLIKELY(qLcEvents().isDebugEnabled()))
            qCDebug(qLcEvents) << "reporting" << tp;

        qCDebug(qLcEvdevTouch) << "Screen rect:" << winRect;
        qCDebug(qLcEvdevTouch) << "Registering touch point:" << tp << tp.state;
    }

    // Let qguiapp pick the target window.
    if (m_filtered)
        emit q->touchPointsUpdated();
    else
        QWindowSystemInterface::handleTouchEvent(nullptr, q->touchDevice(), m_touchPoints);
}

QT_END_NAMESPACE
