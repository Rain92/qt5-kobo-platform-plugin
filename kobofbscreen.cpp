#include "kobofbscreen.h"

#include <QtFbSupport/private/qfbwindow_p.h>

#include <QtGui/QPainter>

#define fastpartialrefreshthreshold 60
#define fastpartialrefreshthreshold2 40
#define TOLERANCE 80

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

KoboFbScreen::KoboFbScreen(const QStringList &args, KoboDeviceDescriptor *koboDevice)
    : koboDevice(koboDevice), mArgs(args), mFbFd(-1), mTtyFd(-1), mBlitter(0)
{
    mMmap.data = 0;
}

KoboFbScreen::~KoboFbScreen()
{
    if (mFbFd != -1)
    {
        fbink_close(mFbFd);
    }

    delete mBlitter;
}

bool KoboFbScreen::initialize()
{
    QRegularExpression fbRx("fb=(.*)");
    QRegularExpression sizeRx("size=(\\d+)x(\\d+)");
    QRegularExpression dpiRx("logicaldpitarget=(\\d+)");

    QString fbDevice;
    QRect userGeometry;
    int logicalDpiTarget = 0;

    // Parse arguments
    for (const QString &arg : qAsConst(mArgs))
    {
        QRegularExpressionMatch match;
        if (arg.contains(sizeRx, &match))
            userGeometry.setSize(QSize(match.captured(1).toInt(), match.captured(2).toInt()));
        else if (arg.contains(fbRx, &match))
            fbDevice = match.captured(1);
        else if (arg.contains(dpiRx, &match))
            logicalDpiTarget = match.captured(1).toInt();
    }

    waitForRefresh = false;
    waveFormFullscreen = WFM_GC16;
    waveFormPartial = WFM_AUTO;
    waveFormFast = WFM_A2;

    if (koboDevice->isREAGL)
    {
        this->waveFormPartial = WFM_REAGLD;
        this->waveFormFast = WFM_DU;
    }
    else if (koboDevice->mark == 7)
    {
        this->waveFormPartial = WFM_REAGL;
        this->waveFormFast = WFM_DU;
    }
    else if (koboDevice->isSunxi)
    {
        this->waveFormPartial = WFM_REAGL;
        this->waveFormFast = WFM_DU;
    }

    fbink_cfg.is_verbose = true;

    fbink_cfg.is_flashing = true;

    // Open framebuffer and keep it around, then setup globals.
    if ((mFbFd = fbink_open()) == EXIT_FAILURE)
    {
        fprintf(stderr, "Failed to open the framebuffer, aborting . . .\n");
        return false;
    }
    if (fbink_init(mFbFd, &fbink_cfg) != EXIT_SUCCESS)
    {
        fprintf(stderr, "Failed to initialize FBInk, aborting . . .\n");
        return false;
    }

    qDebug() << "init done!";

    fbink_fbdata = fbink_get_fb_data(mFbFd);

    mBytesPerLine = fbink_fbdata.fInfo.line_length;
    mFormat = QImage::Format_Grayscale8;
    koboDevice->width = fbink_fbdata.vInfo.xres;
    koboDevice->height = fbink_fbdata.vInfo.yres;
    mGeometry = {0, 0, koboDevice->width, koboDevice->height};

    mPhysicalSize = QSizeF(koboDevice->physicalWidth, koboDevice->physicalHeight);

    if (!fbink_fbdata.isSunxiDevice)
    {
        mMmap.offset = 0;
        mMmap.data = fbink_fbdata.fbPtr;
        mMmap.size = fbink_fbdata.fInfo.smem_len;
    }
    else
    {
        mMmap.offset = 0;
        mMmap.data = fbink_fbdata.fbPtr;
        mMmap.size = fbink_fbdata.sunxiCtx.alloc_size;
    }

    QFbScreen::initializeCompositor();
    mFbScreenImage = QImage(mMmap.data, mGeometry.width(), mGeometry.height(), mBytesPerLine, mFormat);

    qDebug() << "SI isnull:" << mFbScreenImage.isNull() << fbink_fbdata.fInfo.smem_len
             << fbink_fbdata.vInfo.xres * fbink_fbdata.vInfo.yres;

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
    //    this->refreshThread.setPartialRefreshMode(partialRefreshMode);
}

void KoboFbScreen::setFullScreenRefreshMode(WaveForm waveform)
{
    //    this->refreshThread.setFullScreenRefreshMode(waveform);
}

void KoboFbScreen::clearScreen(bool waitForCompleted)
{
    FBInkRect r = {0, 0, static_cast<unsigned short>(mGeometry.width()),
                   static_cast<unsigned short>(mGeometry.height())};
    fbink_cls(mFbFd, &fbink_cfg, &r, true);

    if (waitForCompleted && koboDevice->hasReliableMxcWaitFor)
        fbink_wait_for_complete(mFbFd, *fbink_fbdata.lastMarker);
}

void KoboFbScreen::enableDithering(bool dithering)
{
    //    this->refreshThread.enableDithering(dithering);
}

void KoboFbScreen::doManualRefresh(const QRect &region)
{
    bool isFullRefresh =
        region.width() >= mGeometry.width() - TOLERANCE && region.height() >= mGeometry.height() - TOLERANCE;

    bool isSmall =
        (region.width() < fastpartialrefreshthreshold && region.height() < fastpartialrefreshthreshold) ||
        (region.width() + region.height() < fastpartialrefreshthreshold2);

    if (isFullRefresh)
        fbink_cfg.wfm_mode = this->waveFormFullscreen;
    else if (isSmall)
        fbink_cfg.wfm_mode = this->waveFormFast;
    else
        fbink_cfg.wfm_mode = this->waveFormPartial;

    fbink_cfg.is_flashing = isFullRefresh;

    fbink_refresh(mFbFd, region.top(), region.left(), region.width(), region.height(), &fbink_cfg);

    if (waitForRefresh && koboDevice->hasReliableMxcWaitFor)
        fbink_wait_for_complete(mFbFd, *fbink_fbdata.lastMarker);
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

    doManualRefresh(r);

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
