#ifndef QKOBOFBSCREEN_H
#define QKOBOFBSCREEN_H

#include <QtFbSupport/private/qfbscreen_p.h>

#include "../FBInk/fbink.h"
#include "kobodevicedescriptor.h"
#include "refreshmode.h"

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

    QPixmap grabWindow(WId wid, int x, int y, int width, int height) const override;

    QRegion doRedraw() override;

private:
    KoboDeviceDescriptor *koboDevice;

    QStringList mArgs;
    int mFbFd;
    int mTtyFd;

    QImage mFbScreenImage;
    int mBytesPerLine;

    struct
    {
        uchar *data;
        int offset, size;
    } mMmap;

    FBData fbink_fbdata;

    FBInkConfig fbink_cfg = {0U};

    bool waitForRefresh;
    bool useHardwareDithering;

    WFM_MODE_INDEX_T waveFormFullscreen;
    WFM_MODE_INDEX_T waveFormPartial;
    WFM_MODE_INDEX_T waveFormFast;

    QPainter *mBlitter;
};

#endif  // QKOBOFBSCREEN_H
