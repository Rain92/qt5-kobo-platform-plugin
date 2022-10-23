#ifndef QKOBOFBSCREEN_H
#define QKOBOFBSCREEN_H

#include <QtFbSupport/private/qfbscreen_p.h>
#include <sys/ioctl.h>

#include <cstring>

#include "dither.h"
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

    void setFullScreenRefreshMode(WaveForm waveform);

    void clearScreen(bool waitForCompleted);

    void enableDithering(bool softwareDithering, bool hardwareDithering);

    void doManualRefresh(const QRect &region);

    bool setScreenRotation(ScreenRotation r, int bpp = 8);

    void doSunxiPenRefresh();

    ScreenRotation getScreenRotation();

    FBInkConfig *getFBInkConfig();
    FBInkState *getFBInkState();

    QRegion doRedraw() override;


private:
    void ditherRegion(const QRect &region);

    KoboDeviceDescriptor *koboDevice;

    QStringList mArgs;
    int mFbFd;
    bool debug;

    QImage mFbScreenImage;
    int mBytesPerLine;
    QImage mScreenImageDither;

    QPainter *mBlitter;

    FBInkState fbink_state;

    struct
    {
        unsigned char *bufferPtr;
        size_t bufferSize;
    } memmapInfo;

    FBInkConfig fbink_cfg;

    bool waitForRefresh;
    bool useHardwareDithering;
    bool useSoftwareDithering;

    WFM_MODE_INDEX_T waveFormFullscreen;
    WFM_MODE_INDEX_T waveFormPartial;
    WFM_MODE_INDEX_T waveFormFast;

    int originalRotation;
    int originalBpp;
};

#endif  // QKOBOFBSCREEN_H
