#include "einkrefreshthread.h"

#define fastpartialrefreshthreshold 60
#define fastpartialrefreshthreshold2 40
#define TOLERANCE 80

EinkrefreshThread::EinkrefreshThread()
    : exitFlag(), fb(), screenRect(), marker(), waitCompleted(), partialRefreshMode(), dithering()
{
}

EinkrefreshThread::~EinkrefreshThread()
{
    doExit();
}

void EinkrefreshThread::initialize(int fb, FBInkKoboSunxi* sunxiCtx, KoboDeviceDescriptor* koboDevice,
                                   int marker, bool waitCompleted, PartialRefreshMode partialRefreshMode,
                                   bool dithering)
{
    this->fb = fb;
    this->screenRect = QRect(0, 0, koboDevice->width, koboDevice->height);
    this->marker = marker;
    this->partialRefreshMode = partialRefreshMode;
    this->dithering = dithering;

    this->waveFormFullscreen = WaveForm_GC16;
    this->waveFormPartial = WaveForm_AUTO;
    this->waveFormFast = WaveForm_A2;

    this->koboDevice = koboDevice;

    this->sunxiCtx = 0;

    if (koboDevice->isREAGL)
    {
        this->waveFormPartial = WaveForm_REAGLD;
        this->waveFormFast = WaveForm_DU;
    }
    else if (koboDevice->mark == 7)
    {
        this->waveFormPartial = WaveForm_REAGL;
        this->waveFormFast = WaveForm_DU;
    }
    else if (koboDevice->isSunxi)
    {
        this->waveFormPartial = WaveForm_REAGL;
        this->waveFormFast = WaveForm_DU;

        this->sunxiCtx = sunxiCtx;
    }

    this->waitCompleted = waitCompleted && koboDevice->hasReliableMxcWaitFor;

    this->start();
}

void EinkrefreshThread::setPartialRefreshMode(PartialRefreshMode partialRefreshMode)
{
    this->partialRefreshMode = partialRefreshMode;
}

void EinkrefreshThread::setFullScreenRefreshMode(WaveForm waveform)
{
    this->waveFormFullscreen = waveform;
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

            if (koboDevice->mark <= 6)
                refreshScreenRegion(fb, region, NTX_WFM_MODE_INIT, UPDATE_MODE_FULL, marker);
            else if (koboDevice->mark == 7)
                refreshScreenRegionMk7(fb, region, NTX_WFM_MODE_INIT, UPDATE_MODE_FULL, 0, marker);
            else if (koboDevice->isSunxi)
                refreshScreenRegionSunxi(*sunxiCtx, region, NTX_WFM_MODE_INIT, UPDATE_MODE_FULL, 0, marker);

            if (waitForCompleted)
            {
                if (koboDevice->mark <= 6)
                    waitRefreshComplete(fb, marker);
                else if (koboDevice->mark == 7)
                    waitRefreshCompleteMk7(fb, marker);
                else if (koboDevice->isSunxi)
                    waitRefreshCompleteSunxi(*sunxiCtx, marker);
            }
        }
    }
}

void EinkrefreshThread::enableDithering(bool dithering)
{
    this->dithering = dithering;
}

void EinkrefreshThread::refresh(const QRect& r)
{
    if (r.width() == 0 || r.height() == 0)
        return;

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

                    uint32_t partialWaveform = fastPartialRefresh ? waveFormFast : waveFormPartial;
                    uint32_t actualWaveform = isFullRefresh ? waveFormFullscreen : partialWaveform;
                    uint32_t updateMode = isFullRefresh ? UPDATE_MODE_FULL : UPDATE_MODE_PARTIAL;

                    //                    qDebug() << "Refreshmode: " << fastPartialRefresh << isFullRefresh
                    //                    << actualWaveform << r;

                    if (koboDevice->mark <= 6)
                        refreshScreenRegion(fb, region, actualWaveform, updateMode, marker);
                    else if (koboDevice->mark == 7)
                        refreshScreenRegionMk7(fb, region, actualWaveform, updateMode, 0, marker);
                    else if (koboDevice->isSunxi)
                        refreshScreenRegionSunxi(*sunxiCtx, region, actualWaveform, updateMode, 0, marker);

                    if (waitCompleted)
                    {
                        if (koboDevice->mark <= 6)
                            waitRefreshComplete(fb, marker);
                        else if (koboDevice->mark == 7)
                            waitRefreshCompleteMk7(fb, marker);
                        else if (koboDevice->isSunxi)
                            waitRefreshCompleteSunxi(*sunxiCtx, marker);
                    }
                }
                else
                    break;
            }
        }
    }
}
