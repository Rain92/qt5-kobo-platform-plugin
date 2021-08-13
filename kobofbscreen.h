#ifndef QKOBOFBSCREEN_H
#define QKOBOFBSCREEN_H

#include <QtFbSupport/private/qfbscreen_p.h>
#include <sys/ioctl.h>

#include "fbink.h"
#include "kobodevicedescriptor.h"
#include "refreshmode.h"

enum ScreenRotation
{
    RotationUR = FB_ROTATE_UR,
    RotationCW = FB_ROTATE_CW,
    RotationUD = FB_ROTATE_UD,
    RotationCCW = FB_ROTATE_CCW
};

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

    QPixmap grabWindow(WId wid, int x, int y, int width, int height) const override;

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
