#include "einkrefreshthread.h"

#include <algorithm>  // std::min
#define fastpartialrefreshthreshold 60
#define fastpartialrefreshthreshold2 40
#define TOLERANCE 80

EinkrefreshThread::EinkrefreshThread()
    : EinkrefreshThread(0, {}, 0, false, AccuratePartialRefresh, WaveForm_GC16, true)
{
}

EinkrefreshThread::EinkrefreshThread(int fb, QRect screenRect, int marker, bool waitCompleted,
                                     PartialRefreshMode partialRefreshMode, WaveForm fullscreenWaveForm,
                                     bool dithering)
    : exitFlag(0),
      fb(fb),
      screenRect(screenRect),
      marker(marker),
      waitCompleted(waitCompleted),
      partialRefreshMode(partialRefreshMode),
      fullscreenWaveForm(fullscreenWaveForm),
      dithering(dithering)
{
}

EinkrefreshThread::~EinkrefreshThread()
{
    doExit();
}

void EinkrefreshThread::initialize(int fb, QRect screenRect, int marker, bool waitCompleted,
                                   PartialRefreshMode partialRefreshMode, WaveForm fullscreenWaveForm,
                                   bool dithering)
{
    this->fb = fb;
    this->screenRect = screenRect;
    this->marker = marker;
    this->waitCompleted = waitCompleted;
    this->partialRefreshMode = partialRefreshMode;
    this->fullscreenWaveForm = fullscreenWaveForm;
    this->dithering = dithering;

    this->start();
}

void EinkrefreshThread::setPartialRefreshMode(PartialRefreshMode partialRefreshMode)
{
    this->partialRefreshMode = partialRefreshMode;
}

void EinkrefreshThread::setFullScreenRefreshMode(WaveForm waveform)
{
    this->fullscreenWaveForm = waveform;
}

void EinkrefreshThread::clearScreen(bool waitForCompleted)
{
    if (!exitFlag)
    {
        mutexWaitCondition.lock();
        waitCondition.wait(&mutexWaitCondition);
        mutexWaitCondition.unlock();

        if (!exitFlag)
        {
            mxcfb_rect region;
            region.top = screenRect.top();
            region.left = screenRect.left();
            region.width = screenRect.width();
            region.height = screenRect.height();

            refreshScreenRegion(fb, region, NTX_WFM_MODE_INIT, UPDATE_MODE_FULL, marker, 0);

            if (waitForCompleted)
                waitRefreshComplete(fb, marker);
        }
    }
}

void EinkrefreshThread::enableDithering(bool dithering)
{
    this->dithering = dithering;
}

void EinkrefreshThread::refresh(const QRect& r)
{
    mutexQueue.lock();
    for (int i = 0; i < queue.size(); i++)
    {
        auto& r2 = queue.at(i);
        if (r.contains(r2))
        {
            queue.removeAt(i);
            i--;
        }
        else if (r2.contains(r))
        {
            mutexQueue.unlock();
            return;
        }
    }
    queue.enqueue(r);
    mutexQueue.unlock();
    waitCondition.wakeAll();
    //    qDebug() << "scheduled:" << r;
}

void EinkrefreshThread::doExit()
{
    exitFlag = 1;
    waitCondition.wakeAll();
    wait();
}

void EinkrefreshThread::run()
{
    while (!exitFlag)
    {
        mutexWaitCondition.lock();
        waitCondition.wait(&mutexWaitCondition);
        mutexWaitCondition.unlock();

        if (!exitFlag)
        {
            while (true)
            {
                mutexQueue.lock();
                QRect r(queue.isEmpty() ? QRect() : queue.dequeue());
                mutexQueue.unlock();

                if (!r.isNull())
                {
                    bool isFullRefresh = r.width() >= screenRect.width() - TOLERANCE &&
                                         r.height() >= screenRect.height() - TOLERANCE;

                    if (isFullRefresh)
                        r = screenRect;

                    mxcfb_rect region;

                    region.top = r.top();
                    region.left = r.left();
                    region.width = r.width();
                    region.height = r.height();

                    bool isSmall = (r.width() < fastpartialrefreshthreshold &&
                                    r.height() < fastpartialrefreshthreshold) ||
                                   (r.width() + r.height() < fastpartialrefreshthreshold2);

                    bool fastPartialRefresh = partialRefreshMode == FastPartialRefresh ||
                                              (partialRefreshMode == MixedPartialRefresh && isSmall);

                    int actualFullscreenWaveForm;
                    switch (fullscreenWaveForm)
                    {
                        case WaveForm_AUTO:
                            actualFullscreenWaveForm = WAVEFORM_MODE_AUTO;
                            break;
                        case WaveForm_A2:
                            actualFullscreenWaveForm = NTX_WFM_MODE_A2;
                            break;
                        case WaveForm_GC4:
                            actualFullscreenWaveForm = NTX_WFM_MODE_GC4;
                            break;
                        case WaveForm_GC16:
                            actualFullscreenWaveForm = NTX_WFM_MODE_GC16;
                            break;
                        case WaveForm_GL16:
                            actualFullscreenWaveForm = NTX_WFM_MODE_GL16;
                            break;
                        // unsupported for now
                        case WaveForm_REAGL:
                        case WaveForm_REAGLD:
                        default:
                            actualFullscreenWaveForm = NTX_WFM_MODE_GC16;
                            break;
                    }

                    uint32_t partialWaveform = fastPartialRefresh ? WAVEFORM_MODE_A2 : WAVEFORM_MODE_AUTO;
                    uint32_t actualWaveform = isFullRefresh ? actualFullscreenWaveForm : partialWaveform;
                    uint32_t updateMode = isFullRefresh ? UPDATE_MODE_FULL : UPDATE_MODE_PARTIAL;
                    uint32_t flags = dithering ? EPDC_FLAG_USE_DITHERING_Y4 : 0;

                    //                    qDebug() << "Refreshmode: " << fastPartialRefresh << isFullRefresh
                    //                    << actualWaveform << r;
                    refreshScreenRegion(fb, region, actualWaveform, updateMode, marker, flags);

                    if (waitCompleted)
                        waitRefreshComplete(fb, marker);
                }
                else
                    break;
            }
        }
    }
}
