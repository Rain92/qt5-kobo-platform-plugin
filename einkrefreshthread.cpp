#include "einkrefreshthread.h"

#include <algorithm>  // std::min
#define fastpartialrefreshthreshold 60
#define fastpartialrefreshthreshold2 40

EinkrefreshThread::EinkrefreshThread() : EinkrefreshThread(0, 0, 0, 0, false, AccuratePartialRefresh) {}

EinkrefreshThread::EinkrefreshThread(int fb, int width, int height, int marker, bool wait_completed,
                                     PartialRefreshMode partial_refresh_mode)
    : exit_flag(0),
      fb(fb),
      width(width),
      height(height),
      marker(marker),
      wait_completed(wait_completed),
      partial_refresh_mode(partial_refresh_mode)
{
}

EinkrefreshThread::~EinkrefreshThread()
{
    doExit();
}

void EinkrefreshThread::initialize(int fb, int width, int height, int marker, bool wait_completed,
                                   PartialRefreshMode partial_refresh_mode)
{
    this->fb = fb;
    this->width = width;
    this->height = height;
    this->marker = marker;
    this->wait_completed = wait_completed;
    this->partial_refresh_mode = partial_refresh_mode;

    this->start();
}

void EinkrefreshThread::setPartialRefreshMode(PartialRefreshMode partial_refresh_mode)
{
    this->partial_refresh_mode = partial_refresh_mode;
}

void EinkrefreshThread::refresh(const QRect& r)
{
    mutex_queue.lock();
    queue.enqueue(r);
    mutex_queue.unlock();
    waitcondition.wakeAll();
}

void EinkrefreshThread::doExit()
{
    exit_flag = 1;
    waitcondition.wakeAll();
    wait();
}

void EinkrefreshThread::run()
{
    while (!exit_flag)
    {
        mutex_waitcondition.lock();
        waitcondition.wait(&mutex_waitcondition);
        mutex_waitcondition.unlock();

        if (!exit_flag)
        {
            while (true)
            {
                mutex_queue.lock();
                QRect r(queue.isEmpty() ? QRect() : queue.dequeue());
                mutex_queue.unlock();

                if (!r.isNull())
                {
                    bool full_refresh = r.width() == width && r.height() == height;

                    struct mxcfb_rect region;

                    region.top = r.top();
                    region.left = r.left();
                    region.width = r.width();
                    region.height = r.height();

                    bool is_small = (r.width() < fastpartialrefreshthreshold &&
                                     r.height() < fastpartialrefreshthreshold) ||
                                    (r.width() + r.height() < fastpartialrefreshthreshold2);

                    bool fast_partial_refresh = partial_refresh_mode == FastPartialRefresh ||
                                                (partial_refresh_mode == MixedPartialRefresh && is_small);

                    uint32_t partial_waveform = fast_partial_refresh ? WAVEFORM_MODE_A2 : WAVEFORM_MODE_AUTO;
                    uint32_t waveform_mode = full_refresh ? NTX_WFM_MODE_GC16 : partial_waveform;
                    uint32_t update_mode = full_refresh ? UPDATE_MODE_FULL : UPDATE_MODE_PARTIAL;

                    refreshScreenRegion(fb, region, waveform_mode, update_mode, marker);

                    if (wait_completed)
                        waitRefreshComplete(fb, marker);
                }
                else
                    break;
            }
        }
    }
}
