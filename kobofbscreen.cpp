#include "kobofbscreen.h"

#include <QtFbSupport/private/qfbwindow_p.h>

#include <QtGui/QPainter>

#define fastpartialrefreshthreshold 60
#define fastpartialrefreshthreshold2 40
#define TOLERANCE 80

static bool getScreenInfo(int fbfd, fb_var_screeninfo &vInfo, fb_fix_screeninfo &fInfo)
{
    if (ioctl(fbfd, FBIOGET_FSCREENINFO, &fInfo) != 0)
    {
        qErrnoWarning(errno, "Error reading fixed information");
        return false;
    }

    if (ioctl(fbfd, FBIOGET_VSCREENINFO, &vInfo))
    {
        qErrnoWarning(errno, "Error reading variable information");
        return false;
    }

    return true;
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

static bool setScreenInfo(int fbFd, uint32_t bpp, int8_t rota, fb_var_screeninfo &vInfo, bool isKindleLegacy)
{
    if (bpp >= 8)
        vInfo.bits_per_pixel = bpp;

        // NOTE: We have to counteract the rotation shenanigans the Kernel might be enforcing...
        //       c.f., mxc_epdc_fb_check_var @ drivers/video/mxc/mxc_epdc_fb.c OR
        //       drivers/video/fbdev/mxc/mxc_epdc_v2_fb.c The goal being to end up in the *same* effective
        //       rotation as before.
        // First, remember the current rotation as the expected one...
#if defined(FBINK_FOR_KOBO) || defined(FBINK_FOR_CERVANTES) || defined(FBINK_FOR_KINDLE)
    uint32_t expected_rota = vInfo.rotate;
#endif
    // Then, set the requested rotation, if there was one...
    if (rota != -1)
    {
        vInfo.rotate = (uint32_t)rota;
        // And flag it as the expected rota for the sanity checks
#if defined(FBINK_FOR_KOBO) || defined(FBINK_FOR_CERVANTES) || defined(FBINK_FOR_KINDLE)
        expected_rota = (uint32_t)rota;
#endif
    }

    if (ioctl(fbFd, FBIOPUT_VSCREENINFO, &vInfo))
    {
        perror("ioctl PUT_V");
        return false;
    }

#ifdef FBINK_FOR_KINDLE
    // Deal once again with einkfb properly...
    if (isKindleLegacy)
    {
        orientation_t orientation = linuxfb_rotate_to_einkfb_orientation(expected_rota);
        if (ioctl(fbFd, FBIO_EINK_SET_DISPLAY_ORIENTATION, orientation))
        {
            perror("ioctl FBIO_EINK_SET_DISPLAY_ORIENTATION");
            return false;
        }
    }
#endif

#if defined(FBINK_FOR_KOBO) || defined(FBINK_FOR_CERVANTES)
    // NOTE: Double-check that we weren't bit by rotation quirks...
    if (vInfo.rotate != expected_rota)
    {
        // Brute-force it until it matches...
        for (uint32_t i = vInfo.rotate, j = FB_ROTATE_UR; j <= FB_ROTATE_CCW; i = (i + 1U) & 3U, j++)
        {
            // If we finally got the right orientation, break the loop
            if (vInfo.rotate == expected_rota)
            {
                break;
            }
            // Do the i -> i + 1 -> i dance to be extra sure...
            // (This is useful on devices where the kernel *always* switches to the invert orientation, c.f.,
            // rota.c)
            vInfo.rotate = i;
            if (ioctl(fbFd, FBIOPUT_VSCREENINFO, &vInfo))
            {
                perror("ioctl PUT_V");
                return false;
            }

            // Don't do anything extra if that was enough...
            if (vInfo.rotate == expected_rota)
            {
                continue;
            }
            // Now for i + 1 w/ wraparound, since the valid rotation range is [0..3] (FB_ROTATE_UR to
            // FB_ROTATE_CCW). (i.e., a Portrait/Landscape swap to counteract potential side-effects of a
            // kernel-side mandatory invert)
            uint32_t n = (i + 1U) & 3U;
            vInfo.rotate = n;
            if (ioctl(fbFd, FBIOPUT_VSCREENINFO, &vInfo))
            {
                perror("ioctl PUT_V");
                return false;
            }

            // And back to i, if need be...
            if (vInfo.rotate == expected_rota)
            {
                continue;
            }
            vInfo.rotate = i;
            if (ioctl(fbFd, FBIOPUT_VSCREENINFO, &vInfo))
            {
                perror("ioctl PUT_V");
                return false;
            }
        }
    }
#endif

#ifdef FBINK_FOR_KINDLE
    // And, again, einkfb is a special snowflake...
    if (isKindleLegacy)
    {
        orientation_t orientation = orientation_portrait;
        if (ioctl(fbFd, FBIO_EINK_GET_DISPLAY_ORIENTATION, &orientation))
        {
            perror("ioctl FBIO_EINK_GET_DISPLAY_ORIENTATION");
            return false;
        }
    }
#endif
    return true;
}

KoboFbScreen::KoboFbScreen(const QStringList &args, KoboDeviceDescriptor *koboDevice)
    : koboDevice(koboDevice), mArgs(args), mFbFd(-1), mBlitter(0)
{
    mMmap.data = 0;
    i2c_smbus_read_byte_data(0, 0);
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

    fbink_cfg.is_verbose = false;

    // Open framebuffer and keep it around, then setup globals.
    if ((mFbFd = fbink_open()) == EXIT_FAILURE)
    {
        qDebug() << stderr << "Failed to open the framebuffer";
        return false;
    }

    if (fbink_init(mFbFd, &fbink_cfg) != EXIT_SUCCESS)
    {
        qDebug() << stderr << "Failed to initialize FBInk.";
        return false;
    }

    if (!getScreenInfo(mFbFd, vInfo, fInfo))
    {
        qDebug() << "Failed to get fb info";
        return false;
    }

    mDepth = determineDepth(vInfo);
    mFormat = determineFormat(vInfo, mDepth);

    setScreenRotation(RotationUR);

    QFbScreen::initializeCompositor();

    qDebug() << "setting rotation" << vInfo.rotate;
    qDebug() << "SI isnull:" << mFbScreenImage.isNull() << fInfo.smem_len << vInfo.xres * vInfo.yres;

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
    if (!getScreenInfo(mFbFd, vInfo, fInfo))
    {
        qDebug() << "Failed to get fb info";
        return false;
    }

    if (!setScreenInfo(mFbFd, 0, fbink_rota_canonical_to_native(r), vInfo, fbink_state.is_kindle_legacy))
    {
        qDebug() << "Failed to set fb info";
        return false;
    }

    fbink_reinit(mFbFd, &fbink_cfg);

    fbink_get_state(&fbink_cfg, &fbink_state);

    mBytesPerLine = fInfo.line_length;
    koboDevice->width = fbink_state.screen_width;
    koboDevice->height = fbink_state.screen_height;
    mGeometry = {0, 0, koboDevice->width, koboDevice->height};

    mPhysicalSize = QSizeF(koboDevice->physicalWidth, koboDevice->physicalHeight);

    if (fbink_get_fb_data(mFbFd, &fbink_fbdata) != EXIT_SUCCESS)
    {
        qDebug() << stderr << "Failed to get fb data or memmap screen";
        return false;
    }

    mMmap.offset = 0;
    mMmap.data = fbink_fbdata.fbPtr;
    mMmap.size = fbink_fbdata.allocationSize;

    mFbScreenImage = QImage(mMmap.data, mGeometry.width(), mGeometry.height(), mBytesPerLine, mFormat);

    return true;
}

ScreenRotation KoboFbScreen::getScreenRotation()
{
    if (!getScreenInfo(mFbFd, vInfo, fInfo))
        return RotationUR;

    return (ScreenRotation)fbink_rota_native_to_canonical(vInfo.rotate);
}

void KoboFbScreen::setPartialRefreshMode(PartialRefreshMode partialRefreshMode)
{
    //    this->refreshThread.setPartialRefreshMode(partialRefreshMode);
}

void KoboFbScreen::setFullScreenRefreshMode(WaveForm waveform)
{
    this->waveFormFullscreen = waveform;
}

void KoboFbScreen::clearScreen(bool waitForCompleted)
{
    //    FBInkRect r = {0, 0, static_cast<unsigned short>(mGeometry.width()),
    //                   static_cast<unsigned short>(mGeometry.height())};
    //    fbink_cls(mFbFd, &fbink_cfg, &r, true);

    //    if (waitForCompleted && koboDevice->hasReliableMxcWaitFor)
    //        fbink_wait_for_complete(mFbFd, LAST_MARKER);
}

void KoboFbScreen::enableDithering(bool dithering)
{
    useHardwareDithering = dithering;
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
        fbink_wait_for_complete(mFbFd, LAST_MARKER);
}

QRegion KoboFbScreen::doRedraw()
{
    //    QElapsedTimer t;
    //    t.start();
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

    //    qDebug() << "redrawing region" << r << "in" << t.elapsed();

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
