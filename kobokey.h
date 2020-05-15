#ifndef KOBOKEY_H
#define KOBOKEY_H

#include <QtCore>

enum KoboKey
{
    Key_Light = Qt::Key_BrightnessAdjust,
    Key_Home = Qt::Key_Home,
    Key_Power = Qt::Key_PowerDown,
    Key_SleepCover = Qt::Key_PowerOff + 10000,
    Key_PagePackward = Qt::Key_PageUp,
    Key_PageForward = Qt::Key_PageDown
};

#endif  // KOBOKEY_H
