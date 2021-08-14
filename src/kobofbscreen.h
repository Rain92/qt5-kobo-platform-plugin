#ifndef QKOBOFBSCREEN_H
#define QKOBOFBSCREEN_H

#include <QtFbSupport/private/qfbscreen_p.h>
#include <sys/ioctl.h>

#include "eink/mxcfb-kobo.h"
#include "einkenums.h"
#include "fbink.h"
#include "kobodevicedescriptor.h"

class QPainter;
class QFbCursor;

class KoboFbScreen : public QFbScreen
{
    Q_OBJECT
public:
    KoboFbScreen(const QStringList &args, KoboDeviceDescriptor *koboDevice);
    ~KoboFbScreen();

    bool initialize() override;

    void setPartialRefreshMode(PartialRefreshMode partialRefreshMode);

    void setFullScreenRefreshMode(WaveForm waveform);
    void clearScreen(bool waitForCompleted);

    void enableDithering(bool dithering);

    void doManualRefresh(const QRect &region);

    bool setScreenRotation(ScreenRotation r);
    ScreenRotation getScreenRotation();

    QRegion doRedraw() override;

private:
    KoboDeviceDescriptor *koboDevice;

    QStringList mArgs;
    int mFbFd;

    QImage mFbScreenImage;
    int mBytesPerLine;

    FBInkState fbink_state;
    FBPtrInfo fbinkFbInfo;

    FBInkConfig fbink_cfg;

    bool waitForRefresh;
    bool useHardwareDithering;

    WFM_MODE_INDEX_T waveFormFullscreen;
    WFM_MODE_INDEX_T waveFormPartial;
    WFM_MODE_INDEX_T waveFormFast;

    QPainter *mBlitter;
};

#endif  // QKOBOFBSCREEN_H
