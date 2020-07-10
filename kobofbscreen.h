#ifndef QLINUXFBSCREEN_H
#define QLINUXFBSCREEN_H

#include <QtFbSupport/private/qfbscreen_p.h>

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

    void setPartialRefreshMode(PartialRefreshMode partial_refresh_mode);

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

    QPainter *mBlitter;
};

#endif  // QLINUXFBSCREEN_H
