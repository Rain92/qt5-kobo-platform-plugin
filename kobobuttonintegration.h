#ifndef KOBOKB_H
#define KOBOKB_H

#include <fcntl.h>
#include <linux/input.h>
#include <qevent.h>
#include <qguiapplication.h>
#include <unistd.h>

#include <QDebug>
#include <QSocketNotifier>
#include <iostream>

#include "kobokey.h"

class KoboButtonIntegration : public QObject
{
    Q_OBJECT

public:
    KoboButtonIntegration(QObject* parent = nullptr, const char* inputDevice = "/dev/input/event0",
                          bool debug = false);
    ~KoboButtonIntegration();

private slots:
    void activity(int);

private:
    int inputHandle;
    QSocketNotifier* socketNotifier;

    bool debug;
    bool isInputCaptured;

    void captureInput();
    void releaseInput();
};

#endif  // KOBOKB_H
