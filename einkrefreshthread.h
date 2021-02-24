#ifndef EINKREFRESHTHREAD_H
#define EINKREFRESHTHREAD_H

#include <QMutex>
#include <QQueue>
#include <QRect>
#include <QThread>
#include <QTime>
#include <QWaitCondition>

#include "eink.h"
#include "refreshmode.h"

class EinkrefreshThread : public QThread
{
public:
    EinkrefreshThread();
    EinkrefreshThread(int fb, QRect screenRect, int marker, bool waitCompleted,
                      PartialRefreshMode partialRefreshMode, WaveForm fullscreenWaveForm, bool dithering);
    ~EinkrefreshThread();

    void initialize(int fb, QRect screenRect, int marker, bool waitCompleted,
                    PartialRefreshMode partialRefreshMode, WaveForm fullscreenWaveForm, bool dithering);

    void setPartialRefreshMode(PartialRefreshMode partialRefreshMode);

    void setFullScreenRefreshMode(WaveForm waveform);
    void clearScreen(bool waitForCompleted);

    void enableDithering(bool dithering);

    void refresh(const QRect& r);
    void doExit();

protected:
    virtual void run();

private:
    QWaitCondition waitCondition;
    QQueue<QRect> queue;
    QMutex mutexWaitCondition;
    QMutex mutexQueue;

    char exitFlag;
    int fb;
    QRect screenRect;
    unsigned int marker;
    bool waitCompleted;
    PartialRefreshMode partialRefreshMode;
    WaveForm fullscreenWaveForm;
    bool dithering;
};

#endif  // EINKREFRESHTHREAD_H
