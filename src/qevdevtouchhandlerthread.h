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

#ifndef QEVDEVTOUCHHANDLERTHREAD_H
#define QEVDEVTOUCHHANDLERTHREAD_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <QtCore/private/qthread_p.h>
#include <qpa/qwindowsysteminterface.h>

#include <QList>
#include <QObject>
#include <QString>
#include <QThread>

#include "qevdevtouchdata.h"
#include "qevdevtouchhandler.h"

QT_BEGIN_NAMESPACE

class QEvdevTouchScreenHandlerThread : public QDaemonThread
{
    Q_OBJECT
public:
    explicit QEvdevTouchScreenHandlerThread(const QString &device, const QString &spec,
                                            QObject *parent, KoboFbScreen *koboFbScreen);
    ~QEvdevTouchScreenHandlerThread();
    void run() override;

    bool isTouchDeviceRegistered() const;

    bool eventFilter(QObject *object, QEvent *event) override;

    void scheduleTouchPointUpdate();

signals:
    void touchDeviceRegistered();

private:
    Q_INVOKABLE void notifyTouchDeviceRegistered();

    void filterAndSendTouchPoints();
    QRect targetScreenGeometry() const;

    QString m_device;
    QString m_spec;
    QEvdevTouchScreenHandler *m_handler;
    bool m_touchDeviceRegistered;

    bool m_touchUpdatePending;
    QWindow *m_filterWindow;

    struct FilteredTouchPoint
    {
        QEvdevTouchFilter x;
        QEvdevTouchFilter y;
        QWindowSystemInterface::TouchPoint touchPoint;
    };
    QHash<int, FilteredTouchPoint> m_filteredPoints;

    float m_touchRate;

    KoboFbScreen *koboFbScreen;
};

QT_END_NAMESPACE

#endif  // QEVDEVTOUCH_P_H
