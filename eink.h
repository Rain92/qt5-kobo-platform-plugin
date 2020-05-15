#include <stdlib.h>

#include <QDebug>

#include "mxcfb-kobo.h"
#include "sys/ioctl.h"

static int refreshScreenRegion(int fbfd, const struct mxcfb_rect region, uint32_t waveform_mode, uint32_t update_mode,
                               uint32_t marker)
{
    struct mxcfb_update_data_v1_ntx update = {
        .update_region = region,
        .waveform_mode = waveform_mode,
        .update_mode = update_mode,
        .update_marker = marker,
        .temp = TEMP_USE_AMBIENT,
        .flags = (waveform_mode == WAVEFORM_MODE_REAGLD)
                     ? EPDC_FLAG_USE_AAD
                     : (waveform_mode == WAVEFORM_MODE_A2) ? EPDC_FLAG_FORCE_MONOCHROME : 0U,
        .alt_buffer_data = {0U, 0, 0, 0, {0, 0, 0, 0}}};

    int rv = ioctl(fbfd, MXCFB_SEND_UPDATE_V1_NTX, &update);

    if (rv < 0)
    {
        qDebug() << "Screen refresh failed!"
                 << "Region:" << region.top << region.left << region.width << region.height;

        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}

static int waitRefreshComplete(int fbfd, uint32_t marker)
{
    int rv = ioctl(fbfd, MXCFB_WAIT_FOR_UPDATE_COMPLETE_V1, &marker);

    if (rv < 0)
    {
        qDebug() << "Wait for screen refresh failed!";
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
