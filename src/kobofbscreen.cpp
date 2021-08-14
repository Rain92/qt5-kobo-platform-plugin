#include "kobofbscreen.h"

#include <QtFbSupport/private/qfbwindow_p.h>

#include <QtGui/QPainter>

// force the compiler to link i2c-tools
extern "C"
{
#include "i2c/smbus.h"
    __s32 (*i2c_smbus_read_byte_fp)(int) = &i2c_smbus_read_byte;
}

#define SMALLTHRESHOLD1 60
#define SMALLTHRESHOLD2 40
#define FULLSCREENTOLERANCE 80

static QImage::Format determineFormat(int fbfd, int depth)
{
    fb_var_screeninfo info;
    if (ioctl(fbfd, FBIOGET_VSCREENINFO, &info))
        return QImage::Format_Invalid;

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
        case 8:
            format = QImage::Format_Grayscale8;
            break;
        default:
            break;
    }

    return format;
}

KoboFbScreen::KoboFbScreen(const QStringList &args, KoboDeviceDescriptor *koboDevice)
    : koboDevice(koboDevice),
      mArgs(args),
      mFbFd(-1),
      debug(false),
      mFbScreenImage(),
      mBytesPerLine(0),
      fbink_state({0}),
      fbinkFbInfo({0}),
      fbink_cfg({0}),
      waitForRefresh(false),
      useHardwareDithering(false),
      useSoftwareDithering(true),
      mBlitter(0)
{
    waitForRefresh = false;
    useHardwareDithering = false;
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
        else if (arg.startsWith("debug"))
            debug = true;
    }

    fbink_cfg.is_verbose = true;

    // Open framebuffer and keep it around, then setup globals.
    if ((mFbFd = fbink_open()) == EXIT_FAILURE)
    {
        qDebug() << "Failed to open the framebuffer";
        return false;
    }

    if (fbink_init(mFbFd, &fbink_cfg) != EXIT_SUCCESS)
    {
        qDebug() << "Failed to initialize FBInk.";
        return false;
    }

    setScreenRotation(RotationUR);

    QFbScreen::initializeCompositor();

    if (mFbScreenImage.isNull())
        qDebug() << "Error, mFbScreenImage is invalid!";

    if (logicalDpiTarget > 0)
    {
        //        qputenv("QT_SCALE_FACTOR_ROUNDING_POLICY ", "PassThrough");
        qputenv("QT_SCREEN_SCALE_FACTORS",
                QString::number(koboDevice->dpi / (double)logicalDpiTarget, 'g', 8).toLatin1());
    }

    return true;
}

bool KoboFbScreen::setScreenRotation(ScreenRotation r)
{
    int8_t rota_native = fbink_rota_canonical_to_native(r);
    int bpp = 8;

    if (fbink_set_fb_info(mFbFd, bpp, rota_native, -1, &fbink_cfg) != EXIT_SUCCESS)
    {
        qDebug() << "Failed to set rotation and bpp.";
        return false;
    }

    fbink_get_state(&fbink_cfg, &fbink_state);

    mBytesPerLine = fbink_state.screen_stride;
    koboDevice->width = fbink_state.screen_width;
    koboDevice->height = fbink_state.screen_height;

    if (debug)
        qDebug() << "screen info:" << fbink_state.screen_width << fbink_state.screen_height
                 << fbink_state.screen_stride << fbink_state.current_rota
                 << fbink_rota_native_to_canonical(fbink_state.current_rota) << fbink_state.bpp;

    mGeometry = {0, 0, koboDevice->width, koboDevice->height};

    mPhysicalSize = QSizeF(koboDevice->physicalWidth, koboDevice->physicalHeight);

    if (fbink_get_fb_pointer(mFbFd, &fbinkFbInfo) != EXIT_SUCCESS)
    {
        qDebug() << "Failed to get fb data or memmap screen";
        return false;
    }

    mDepth = fbink_state.bpp;
    mFormat = determineFormat(mFbFd, mDepth);

    mFbScreenImage = QImage(fbinkFbInfo.fbPtr, mGeometry.width(), mGeometry.height(), mBytesPerLine, mFormat);

    return true;
}

ScreenRotation KoboFbScreen::getScreenRotation()
{
    return (ScreenRotation)fbink_rota_native_to_canonical(fbink_state.current_rota);
}

void KoboFbScreen::setFullScreenRefreshMode(WaveForm waveform)
{
    this->waveFormFullscreen = waveform;
}

void KoboFbScreen::clearScreen(bool waitForCompleted)
{
    FBInkRect r = {0, 0, static_cast<unsigned short>(mGeometry.width()),
                   static_cast<unsigned short>(mGeometry.height())};
    fbink_cls(mFbFd, &fbink_cfg, &r, true);

    if (waitForCompleted && koboDevice->hasReliableMxcWaitFor)
        fbink_wait_for_complete(mFbFd, LAST_MARKER);
}

void KoboFbScreen::enableDithering(bool softwareDithering, bool hardwareDithering)
{
    useHardwareDithering = hardwareDithering;
    useSoftwareDithering = softwareDithering;
}

void KoboFbScreen::ditherRegion(const QRect &region)
{
    if (mScreenImageDither.size() != mScreenImage.size())
        mScreenImageDither = mScreenImage;

    // only dither the lines that were updated
    int updateHeight = region.height();
    int offset = region.top() * mScreenImage.width();

    ditherBuffer(mScreenImageDither.bits() + offset, mScreenImage.bits() + offset, mScreenImage.width(),
                 updateHeight);
    //    ditherFloydSteinberg(mScreenImageDither.bits() + offset, mScreenImage.bits() + offset,
    //                         mScreenImage.width(), updateHeight);
}

void KoboFbScreen::doManualRefresh(const QRect &region)
{
    bool isFullRefresh = region.width() >= mGeometry.width() - FULLSCREENTOLERANCE &&
                         region.height() >= mGeometry.height() - FULLSCREENTOLERANCE;

    bool isSmall = (region.width() < SMALLTHRESHOLD1 && region.height() < SMALLTHRESHOLD1) ||
                   (region.width() + region.height() < SMALLTHRESHOLD2);

    if (isFullRefresh)
        fbink_cfg.wfm_mode = this->waveFormFullscreen;
    else if (isSmall)
        fbink_cfg.wfm_mode = this->waveFormFast;
    else
        fbink_cfg.wfm_mode = this->waveFormPartial;

    fbink_cfg.is_flashing = isFullRefresh;

    fbink_refresh(mFbFd, region.top(), region.left(), region.width(), region.height(), &fbink_cfg);

    if (waitForRefresh && koboDevice->hasReliableMxcWaitFor)
        fbink_wait_for_complete(mFbFd, LAST_MARKER);
}

QRegion KoboFbScreen::doRedraw()
{
    QElapsedTimer t;
    t.start();
    QRegion touched = QFbScreen::doRedraw();

    if (touched.isEmpty())
        return touched;

    QRect r(*touched.begin());
    for (const QRect &rect : touched)
        r = r.united(rect);

    if (!mBlitter)
        mBlitter = new QPainter(&mFbScreenImage);

    if (useSoftwareDithering)
        ditherRegion(r);

    mBlitter->setCompositionMode(QPainter::CompositionMode_Source);
    for (const QRect &rect : touched)
        mBlitter->drawImage(rect, useSoftwareDithering ? mScreenImageDither : mScreenImage, rect);

    doManualRefresh(r);

    if (debug)
        qDebug() << "Painted region" << touched << "in" << t.elapsed() << "ms";

    return touched;
}
