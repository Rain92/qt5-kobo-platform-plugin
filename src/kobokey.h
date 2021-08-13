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
    Key_PageForward = Qt::Key_PageDown,
    Key_Eraser = 0x01010001,
    Key_Highlighter = 0x01010002
};

static const QMap<int, KoboKey> KoboPhysicalKeyMap = {
    {35, Key_SleepCover},   {59, Key_SleepCover}, {90, Key_Light},
    {102, Key_Home},        {116, Key_Power},     {193, Key_PagePackward},
    {194, Key_PageForward}, {331, Key_Eraser},    {332, Key_Highlighter},
};

#endif  // KOBOKEY_H
