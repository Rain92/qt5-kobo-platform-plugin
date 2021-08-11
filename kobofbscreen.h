#ifndef QKOBOFBSCREEN_H
#define QKOBOFBSCREEN_H

#include <QtFbSupport/private/qfbscreen_p.h>

#include "eink_sunxi.h"
#include "einkrefreshthread.h"
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

    void doManualRefresh(QRect region);

    QPixmap grabWindow(WId wid, int x, int y, int width, int height) const override;

    QRegion doRedraw() override;

private:
    EinkrefreshThread refreshThread;

    KoboDeviceDescriptor *koboDevice;

    QStringList mArgs;
    int mFbFd;
    int mTtyFd;

    QImage mFbScreenImage;
    int mBytesPerLine;
    int mOldTtyMode;

    struct
    {
        uchar *data;
        int offset, size;
    } mMmap;

    fb_fix_screeninfo fInfo;
    fb_var_screeninfo vInfo;
    FBInkKoboSunxi sunxiCtx;

    QPainter *mBlitter;
};

#endif  // QKOBOFBSCREEN_H
