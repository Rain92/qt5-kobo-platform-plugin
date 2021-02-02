#ifndef EINKREFRESHTHREAD_H
#define EINKREFRESHTHREAD_H

#include <QMutex>
#include <QQueue>
#include <QRect>
#include <QThread>
#include <QTime>
#include <QWaitCondition>

#include "eink.h"
#include "partialrefreshmode.h"

class EinkrefreshThread : public QThread
{
public:
    EinkrefreshThread();
    EinkrefreshThread(int fb, int width, int height, int marker, bool wait_completed,
                      PartialRefreshMode partial_refresh_mode, bool dithering);
    ~EinkrefreshThread();

    void initialize(int fb, int width, int height, int marker, bool wait_completed,
                    PartialRefreshMode partial_refresh_mode, bool dithering);

    void setPartialRefreshMode(PartialRefreshMode partial_refresh_mode);
    void enableDithering(bool dithering);

    void refresh(const QRect& r);
    void doExit();

protected:
    virtual void run();

private:
    QWaitCondition waitcondition;
    QQueue<QRect> queue;
    QMutex mutex_waitcondition;
    QMutex mutex_queue;

    char exit_flag;
    int fb;
    int width;
    int height;
    unsigned int marker;
    bool wait_completed;
    PartialRefreshMode partial_refresh_mode;
    bool dithering;
};

#endif  // EINKREFRESHTHREAD_H
