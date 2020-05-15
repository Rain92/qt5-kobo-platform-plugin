#ifndef KOBOKB_H
#define KOBOKB_H

#include <fcntl.h>
#include <linux/input.h>

#include <QDebug>
#include <QSocketNotifier>
#include <iostream>

class KoboButtonIntegration : public QObject
{
    Q_OBJECT

public:
    KoboButtonIntegration(QObject* parent = nullptr, bool debug = false);
    virtual ~KoboButtonIntegration();

private slots:
    void activity(int);

private:
    int inputHandle;
    QSocketNotifier* socketNotifier;

    bool debug;
    bool isInputCaptured;

private:
    void captureInput();
    void releaseInput();
};

#endif  // KOBOKB_H
