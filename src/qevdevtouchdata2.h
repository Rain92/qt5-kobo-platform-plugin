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

#ifndef QEVDEVTOUCHDATA2H_H
#define QEVDEVTOUCHDATA2H_H

#include <linux/input.h>
#include <qpa/qwindowsysteminterface.h>

#include "fbink.h"
#include "kobofbscreen.h"
#include "qevdevtouchdata.h"
#include "qevdevtouchfilter_p.h"


#define ABS_MT_SLOT 0x2f

QT_BEGIN_NAMESPACE

class QEvdevTouchScreenHandler;

class QEvdevTouchScreenData2 : public QEvdevTouchScreenData
{
public:
    QEvdevTouchScreenData2(QEvdevTouchScreenHandler *q_ptr, const QStringList &args, KoboFbScreen * koboFbScreen);


    QPointF transformTouchPoint(const QPointF &p, bool up);
    void processInputEvent(input_event *data) override;
    void addTouchPoint(const Contact &contact, Qt::TouchPointStates *combinedStates) override;
private:
    FBInkState* fbink_state;
    KoboFbScreen * koboFbScreen;
};

QT_END_NAMESPACE

#endif  // QEVDEVTOUCH_P_H
