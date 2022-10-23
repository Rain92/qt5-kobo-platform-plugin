/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
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

#ifndef QEVDEVTOUCHDATA_H
#define QEVDEVTOUCHDATA_H

#include <linux/input.h>
#include <qpa/qwindowsysteminterface.h>

#include "qevdevtouchfilter_p.h"


#define ABS_MT_SLOT 0x2f

QT_BEGIN_NAMESPACE

class QEvdevTouchScreenHandler;

class QEvdevTouchScreenData
{
public:
    QEvdevTouchScreenData(QEvdevTouchScreenHandler *q_ptr, const QStringList &args);

    virtual QPointF transformTouchPoint(const QPointF &p);
    virtual void processInputEvent(input_event *data);
    void assignIds();

    QEvdevTouchScreenHandler *q;
    int m_lastEventType;
    QList<QWindowSystemInterface::TouchPoint> m_touchPoints;
    QList<QWindowSystemInterface::TouchPoint> m_lastTouchPoints;

    enum Tool
    {
        UNKNOWN_TOOL,
        FINGER,
        PEN,
    };
    struct Contact
    {
        int trackingId = -1;
        int x = -1;
        int y = -1;
        int maj = -1;
        int pressure = 0;
        Qt::TouchPointState state = Qt::TouchPointPressed;
        QTouchEvent::TouchPoint::InfoFlags flags;
        Tool tool;
    };
    QHash<int, Contact> m_contacts;  // The key is a tracking id for type A, slot number for type B.
    QHash<int, Contact> m_lastContacts;
    Contact m_currentData;
    int m_currentSlot;

    double m_timeStamp;
    double m_lastTimeStamp;

    int findClosestContact(const QHash<int, Contact> &contacts, int x, int y, int *dist);
    virtual void addTouchPoint(const Contact &contact, Qt::TouchPointStates *combinedStates);
    void reportPoints();
    void loadMultiScreenMappings();

    QRect screenGeometry() const;
    void setScreenGeometry(QRect rect);

    int hw_range_x_min;
    int hw_range_x_max;
    int hw_range_y_min;
    int hw_range_y_max;
    int hw_pressure_min;
    int hw_pressure_max;
    QString hw_name;
    QString deviceNode;
    bool m_forceToActiveWindow;
    bool m_typeB;
    int m_swapXY;
    bool m_invertX;
    bool m_invertY;
    int m_rotate;
    bool m_singleTouch;
    bool m_btnTool;
    QString m_screenName;
    QRect m_screenGeometry;

    // Touch filtering and prediction are part of the same thing. The default
    // prediction is 0ms, but sensible results can be achieved by setting it
    // to, for instance, 16ms.
    // For filtering to work well, the QPA plugin should provide a dead-steady
    // implementation of QPlatformWindow::requestUpdate().
    bool m_filtered;
    int m_prediction;

    // When filtering is enabled, protect the access to current and last
    // timeStamp and touchPoints, as these are being read on the gui thread.
    QMutex m_mutex;
};

QT_END_NAMESPACE

#endif  // QEVDEVTOUCH_P_H
