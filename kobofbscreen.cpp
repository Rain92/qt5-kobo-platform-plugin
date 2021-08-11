#include "kobofbscreen.h"

#include <QtFbSupport/private/qfbcursor_p.h>
#include <QtFbSupport/private/qfbwindow_p.h>
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <linux/fb.h>
#include <linux/kd.h>
#include <private/qcore_unix_p.h>  // overrides QT_OPEN
#include <qdebug.h>
#include <qimage.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include <QtCore/QFile>
#include <QtCore/QRegularExpression>
#include <QtGui/QPainter>
#include <array>
#include <cstdio>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>

static int openFramebufferDevice(const QString &dev)
{
    int fd = -1;

    if (access(dev.toLatin1().constData(), R_OK | W_OK) == 0)
        fd = QT_OPEN(dev.toLatin1().constData(), O_RDWR);

    if (fd == -1)
    {
        if (access(dev.toLatin1().constData(), R_OK) == 0)
            fd = QT_OPEN(dev.toLatin1().constData(), O_RDONLY);
    }

    return fd;
}

static int determineDepth(const fb_var_screeninfo &vinfo)
{
    int depth = vinfo.bits_per_pixel;
    if (depth == 24)
    {
        depth = vinfo.red.length + vinfo.green.length + vinfo.blue.length;
        if (depth <= 0)
            depth = 24;  // reset if color component lengths are not reported
    }
    else if (depth == 16)
    {
        depth = vinfo.red.length + vinfo.green.length + vinfo.blue.length;
        if (depth <= 0)
            depth = 16;
    }
    return depth;
}

static QImage::Format determineFormat(const fb_var_screeninfo &info, int depth)
{
    const fb_bitfield rgba[4] = {info.red, info.green, info.blue, info.transp};

    QImage::Format format = QImage::Format_Invalid;

    switch (depth)
    {
        case 32:
        {
            const fb_bitfield argb8888[4] = {{16, 8, 0}, {8, 8, 0}, {0, 8, 0}, {24, 8, 0}};
            const fb_bitfield abgr8888[4] = {{0, 8, 0}, {8, 8, 0}, {16, 8, 0}, {24, 8, 0}};
            if (memcmp(rgba, argb8888, 4 * sizeof(fb_bitfield)) == 0)
            {
                format = QImage::Format_ARGB32;
            }
            else if (memcmp(rgba, argb8888, 3 * sizeof(fb_bitfield)) == 0)
            {
                format = QImage::Format_RGB32;
            }
            else if (memcmp(rgba, abgr8888, 3 * sizeof(fb_bitfield)) == 0)
            {
                format = QImage::Format_RGB32;
                // pixeltype = BGRPixel;
            }
            break;
        }
        case 24:
        {
            const fb_bitfield rgb888[4] = {{16, 8, 0}, {8, 8, 0}, {0, 8, 0}, {0, 0, 0}};
            const fb_bitfield bgr888[4] = {{0, 8, 0}, {8, 8, 0}, {16, 8, 0}, {0, 0, 0}};
            if (memcmp(rgba, rgb888, 3 * sizeof(fb_bitfield)) == 0)
            {
                format = QImage::Format_RGB888;
            }
            else if (memcmp(rgba, bgr888, 3 * sizeof(fb_bitfield)) == 0)
            {
                format = QImage::Format_BGR888;
                // pixeltype = BGRPixel;
            }
            break;
        }
        case 18:
        {
            const fb_bitfield rgb666[4] = {{12, 6, 0}, {6, 6, 0}, {0, 6, 0}, {0, 0, 0}};
            if (memcmp(rgba, rgb666, 3 * sizeof(fb_bitfield)) == 0)
                format = QImage::Format_RGB666;
            break;
        }
        case 16:
        {
            const fb_bitfield rgb565[4] = {{11, 5, 0}, {5, 6, 0}, {0, 5, 0}, {0, 0, 0}};
            const fb_bitfield bgr565[4] = {{0, 5, 0}, {5, 6, 0}, {11, 5, 0}, {0, 0, 0}};
            if (memcmp(rgba, rgb565, 3 * sizeof(fb_bitfield)) == 0)
            {
                format = QImage::Format_RGB16;
            }
            else if (memcmp(rgba, bgr565, 3 * sizeof(fb_bitfield)) == 0)
            {
                format = QImage::Format_RGB16;
                // pixeltype = BGRPixel;
            }
            break;
        }
        case 15:
        {
            const fb_bitfield rgb1555[4] = {{10, 5, 0}, {5, 5, 0}, {0, 5, 0}, {15, 1, 0}};
            const fb_bitfield bgr1555[4] = {{0, 5, 0}, {5, 5, 0}, {10, 5, 0}, {15, 1, 0}};
            if (memcmp(rgba, rgb1555, 3 * sizeof(fb_bitfield)) == 0)
            {
                format = QImage::Format_RGB555;
            }
            else if (memcmp(rgba, bgr1555, 3 * sizeof(fb_bitfield)) == 0)
            {
                format = QImage::Format_RGB555;
                // pixeltype = BGRPixel;
            }
            break;
        }
        case 12:
        {
            const fb_bitfield rgb444[4] = {{8, 4, 0}, {4, 4, 0}, {0, 4, 0}, {0, 0, 0}};
            if (memcmp(rgba, rgb444, 3 * sizeof(fb_bitfield)) == 0)
                format = QImage::Format_RGB444;
            break;
        }
        case 8:
            format = QImage::Format_Grayscale8;
            //            qDebug() << "Using grayscale format.";
            break;
        case 1:
            format = QImage::Format_Mono;  //###: LSB???
            break;
        default:
            break;
    }

    return format;
}

static int openTtyDevice(const QString &device)
{
    const char *const devs[] = {"/dev/tty0", "/dev/tty", "/dev/console", 0};

    int fd = -1;
    if (device.isEmpty())
    {
        for (const char *const *dev = devs; *dev; ++dev)
        {
            fd = QT_OPEN(*dev, O_RDWR);
            if (fd != -1)
                break;
        }
    }
    else
    {
        fd = QT_OPEN(QFile::encodeName(device).constData(), O_RDWR);
    }

    return fd;
}

static void switchToGraphicsMode(int ttyfd, bool doSwitch, int *oldMode)
{
    // Do not warn if the switch fails: the ioctl fails when launching from a
    // remote console and there is nothing we can do about it.  The matching
    // call in resetTty should at least fail then, too, so we do no harm.
    if (ioctl(ttyfd, KDGETMODE, oldMode) == 0)
    {
        if (doSwitch && *oldMode != KD_GRAPHICS)
            ioctl(ttyfd, KDSETMODE, KD_GRAPHICS);
    }
}

static void resetTty(int ttyfd, int oldMode)
{
    ioctl(ttyfd, KDSETMODE, oldMode);

    QT_CLOSE(ttyfd);
}

static void blankScreen(int fd, bool on)
{
    ioctl(fd, FBIOBLANK, on ? VESA_POWERDOWN : VESA_NO_BLANKING);
}

KoboFbScreen::KoboFbScreen(const QStringList &args, KoboDeviceDescriptor *koboDevice)
    : koboDevice(koboDevice), mArgs(args), mFbFd(-1), mTtyFd(-1), mBlitter(0)
{
    mMmap.data = 0;
}

KoboFbScreen::~KoboFbScreen()
{
    if (refreshThread.isRunning())
        refreshThread.doExit();

    if (mFbFd != -1)
    {
        if (koboDevice->mark <= 7)
        {
            if (mMmap.data)
                munmap(mMmap.data - mMmap.offset, mMmap.size);
        }
        else if (koboDevice->isSunxi)
        {
            unmap_ion(mMmap.data, sunxiCtx);
        }

        close(mFbFd);
    }

    if (mTtyFd != -1)
        resetTty(mTtyFd, mOldTtyMode);

    delete mBlitter;
}

bool KoboFbScreen::initialize()
{
    QRegularExpression ttyRx("tty=(.*)");
    QRegularExpression fbRx("fb=(.*)");
    QRegularExpression mmSizeRx("mmsize=(\\d+)x(\\d+)");
    QRegularExpression sizeRx("size=(\\d+)x(\\d+)");
    QRegularExpression offsetRx("offset=(\\d+)x(\\d+)");
    QRegularExpression dpiRx("logicaldpitarget=(\\d+)");

    QString fbDevice, ttyDevice;
    QSize userMmSize;
    QRect userGeometry;
    bool doSwitchToGraphicsMode = true;
    int logicalDpiTarget = 0;

    // Parse arguments
    for (const QString &arg : qAsConst(mArgs))
    {
        QRegularExpressionMatch match;
        if (arg == QLatin1String("nographicsmodeswitch"))
            doSwitchToGraphicsMode = false;
        else if (arg.contains(mmSizeRx, &match))
            userMmSize = QSize(match.captured(1).toInt(), match.captured(2).toInt());
        else if (arg.contains(sizeRx, &match))
            userGeometry.setSize(QSize(match.captured(1).toInt(), match.captured(2).toInt()));
        else if (arg.contains(offsetRx, &match))
            userGeometry.setTopLeft(QPoint(match.captured(1).toInt(), match.captured(2).toInt()));
        else if (arg.contains(ttyRx, &match))
            ttyDevice = match.captured(1);
        else if (arg.contains(fbRx, &match))
            fbDevice = match.captured(1);
        else if (arg.contains(dpiRx, &match))
            logicalDpiTarget = match.captured(1).toInt();
    }

    if (fbDevice.isEmpty())
    {
        fbDevice = QLatin1String("/dev/fb0");
        if (!QFile::exists(fbDevice))
            fbDevice = QLatin1String("/dev/graphics/fb0");
        if (!QFile::exists(fbDevice))
        {
            qWarning("Unable to figure out framebuffer device. Specify it manually.");
            return false;
        }
    }

    // Open the device
    mFbFd = openFramebufferDevice(fbDevice);
    if (mFbFd == -1)
    {
        qErrnoWarning(errno, "Failed to open framebuffer %s", qPrintable(fbDevice));
        return false;
    }

    // Read the fixed and variable screen information
    memset(&vInfo, 0, sizeof(vInfo));
    memset(&fInfo, 0, sizeof(fInfo));

    if (ioctl(mFbFd, FBIOGET_FSCREENINFO, &fInfo) != 0)
    {
        qErrnoWarning(errno, "Error reading fixed information");
        return false;
    }

    if (ioctl(mFbFd, FBIOGET_VSCREENINFO, &vInfo))
    {
        qErrnoWarning(errno, "Error reading variable information");
        return false;
    }

    mDepth = determineDepth(vInfo);
    mBytesPerLine = fInfo.line_length;

    QRect geometry = {0, 0, koboDevice->width, koboDevice->height};
    mGeometry = QRect(QPoint(0, 0), geometry.size());
    mFormat = determineFormat(vInfo, mDepth);

    mPhysicalSize = QSizeF(koboDevice->physicalWidth, koboDevice->physicalHeight);

    if (koboDevice->mark <= 7)
    {
        // mmap the framebuffer
        mMmap.size = fInfo.smem_len;
        uchar *data = (unsigned char *)mmap(0, mMmap.size, PROT_READ | PROT_WRITE, MAP_SHARED, mFbFd, 0);
        if ((long)data == -1)
        {
            qErrnoWarning(errno, "Failed to mmap framebuffer");
            return false;
        }

        mMmap.offset = geometry.y() * mBytesPerLine + geometry.x() * mDepth / 8;
        mMmap.data = data + mMmap.offset;

        QFbScreen::initializeCompositor();
        mFbScreenImage = QImage(mMmap.data, mGeometry.width(), mGeometry.height(), mBytesPerLine, mFormat);
    }
    else if (koboDevice->isSunxi)
    {
        sunxiCtx = setupIonLayer(vInfo);

        fixupSunxiFB(sunxiCtx, fInfo, vInfo);

        unsigned char *fbPtr = 0;

        int res = memmap_ion(fbPtr, sunxiCtx, fInfo);

        qDebug() << "fb info: " << vInfo.xres << vInfo.yres << vInfo.xres_virtual << vInfo.yres_virtual
                 << vInfo.rotate << sunxiCtx.rota << vInfo.bits_per_pixel << fInfo.line_length
                 << fInfo.smem_len;

        koboDevice->width = vInfo.xres;
        koboDevice->height = vInfo.yres;

        mDepth = 8;
        mFormat = QImage::Format_Grayscale8;
        mBytesPerLine = fInfo.line_length;
        mGeometry = {0, 0, koboDevice->width, koboDevice->height};

        if (res == EXIT_FAILURE)
        {
            qErrnoWarning(errno, "Failed to mmap ion buffer!");
            return false;
        }

        mMmap.offset = 0;
        mMmap.data = fbPtr;
        mMmap.size = sunxiCtx.alloc_size;

        QFbScreen::initializeCompositor();
        mFbScreenImage = QImage(mMmap.data, vInfo.xres, vInfo.yres, mBytesPerLine, mFormat);

        qDebug() << "fb info2:" << mGeometry.width() << mGeometry.height() << vInfo.xres << vInfo.yres
                 << mBytesPerLine << fbPtr << mMmap.data;
    }

    //    mCursor = new QFbCursor(this);
    //    mCursor->setOverrideCursor(Qt::BlankCursor);

    mTtyFd = openTtyDevice(ttyDevice);
    if (mTtyFd == -1)
        qErrnoWarning(errno, "Failed to open tty");

    //    if (koboDevice->mark <= 7)
    //    {
    //        switchToGraphicsMode(mTtyFd, doSwitchToGraphicsMode, &mOldTtyMode);
    //        blankScreen(mFbFd, false);
    //    }

    int marker = std::max(getpid(), 123) + 3252;
    bool wait_refresh_completed = false;

    refreshThread.initialize(mFbFd, &sunxiCtx, koboDevice, marker, wait_refresh_completed,
                             PartialRefreshMode::MixedPartialRefresh, true);

    if (logicalDpiTarget > 0)
    {
        //        qputenv("QT_SCALE_FACTOR_ROUNDING_POLICY ", "PassThrough");
        qputenv("QT_SCREEN_SCALE_FACTORS",
                QString::number(koboDevice->dpi / (double)logicalDpiTarget, 'g', 8).toLatin1());
    }

    return true;
}

void KoboFbScreen::setPartialRefreshMode(PartialRefreshMode partialRefreshMode)
{
    this->refreshThread.setPartialRefreshMode(partialRefreshMode);
}

void KoboFbScreen::setFullScreenRefreshMode(WaveForm waveform)
{
    this->refreshThread.setFullScreenRefreshMode(waveform);
}

void KoboFbScreen::clearScreen(bool waitForCompleted)
{
    this->refreshThread.clearScreen(waitForCompleted);
}

void KoboFbScreen::enableDithering(bool dithering)
{
    this->refreshThread.enableDithering(dithering);
}

void KoboFbScreen::doManualRefresh(QRect region)
{
    this->refreshThread.refresh(region);
}

QRegion KoboFbScreen::doRedraw()
{
    QElapsedTimer t;
    t.start();
    QRegion touched = QFbScreen::doRedraw();

    if (touched.isEmpty())
        return touched;

    if (!mBlitter)
        mBlitter = new QPainter(&mFbScreenImage);

    mBlitter->setCompositionMode(QPainter::CompositionMode_Source);
    for (const QRect &rect : touched)
        mBlitter->drawImage(rect, mScreenImage, rect);

    QRect r(*touched.begin());
    for (const QRect &rect : touched)
        r = r.united(rect);

    refreshThread.refresh(r);
    qDebug() << "redrawing region" << r << "in" << t.elapsed();

    return touched;
}

// grabWindow() grabs "from the screen" not from the backingstores.
// In linuxfb's case it will also include the mouse cursor.
QPixmap KoboFbScreen::grabWindow(WId wid, int x, int y, int width, int height) const
{
    if (!wid)
    {
        if (width < 0)
            width = mFbScreenImage.width() - x;
        if (height < 0)
            height = mFbScreenImage.height() - y;
        return QPixmap::fromImage(mFbScreenImage).copy(x, y, width, height);
    }

    QFbWindow *window = windowForId(wid);
    if (window)
    {
        const QRect geom = window->geometry();
        if (width < 0)
            width = geom.width() - x;
        if (height < 0)
            height = geom.height() - y;
        QRect rect(geom.topLeft() + QPoint(x, y), QSize(width, height));
        rect &= window->geometry();
        return QPixmap::fromImage(mFbScreenImage).copy(rect);
    }

    return QPixmap();
}
