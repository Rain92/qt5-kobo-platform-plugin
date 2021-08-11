#include "eink.h"

// Kobo devices ([Mk3<->Mk6])
int refreshScreenRegion(int fbfd, const struct mxcfb_rect region, uint32_t waveform_mode,
                        uint32_t update_mode, uint32_t marker)
{
    struct mxcfb_update_data_v1_ntx update = {
        .update_region = region,
        .waveform_mode = waveform_mode,
        .update_mode = update_mode,
        .update_marker = marker,
        .temp = TEMP_USE_AMBIENT,
        .flags = ((waveform_mode == WAVEFORM_MODE_REAGLD) ? EPDC_FLAG_USE_AAD
                  : (waveform_mode == WAVEFORM_MODE_A2)   ? EPDC_FLAG_FORCE_MONOCHROME
                                                          : 0U),
        .alt_buffer_data = {0U},
    };

    int rv = ioctl(fbfd, MXCFB_SEND_UPDATE_V1_NTX, &update);

    if (rv < 0)
    {
        qDebug() << "Screen refresh failed!"
                 << "Region:" << region.top << region.left << region.width << region.height;

        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}

int waitRefreshComplete(int fbfd, uint32_t marker)
{
    int rv = ioctl(fbfd, MXCFB_WAIT_FOR_UPDATE_COMPLETE_V1, &marker);

    if (rv < 0)
    {
        qDebug() << "Wait for screen refresh failed!";
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}

// Kobo Mark 7 devices ([Mk7])
int refreshScreenRegionMk7(int fbfd, const struct mxcfb_rect region, uint32_t waveform_mode,
                           uint32_t update_mode, int dithering_mode, uint32_t marker)
{
    struct mxcfb_update_data_v2 update = {
        .update_region = region,
        .waveform_mode = waveform_mode,
        .update_mode = update_mode,
        .update_marker = marker,
        .temp = TEMP_USE_AMBIENT,
        .flags = (waveform_mode == WAVEFORM_MODE_GLD16) ? EPDC_FLAG_USE_REGAL
                 : (waveform_mode == WAVEFORM_MODE_A2)  ? EPDC_FLAG_FORCE_MONOCHROME
                                                        : 0U,
        .dither_mode = dithering_mode,
        .quant_bit = (dithering_mode == EPDC_FLAG_USE_DITHERING_PASSTHROUGH)                    ? 0
                     : (waveform_mode == WAVEFORM_MODE_A2 || waveform_mode == WAVEFORM_MODE_DU) ? 1
                     : (waveform_mode == WAVEFORM_MODE_GC4)                                     ? 3
                                                                                                : 7,
        .alt_buffer_data = {0U},
    };

    int rv = ioctl(fbfd, MXCFB_SEND_UPDATE_V2, &update);

    if (rv < 0)
    {
        qDebug() << "Screen refresh failed!"
                 << "Region:" << region.top << region.left << region.width << region.height;

        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}

int waitRefreshCompleteMk7(int fbfd, uint32_t marker)
{
    struct mxcfb_update_marker_data update_marker = {
        .update_marker = marker,
        .collision_test = 0U,
    };
    int rv = ioctl(fbfd, MXCFB_WAIT_FOR_UPDATE_COMPLETE_V3, &update_marker);

    if (rv < 0)
    {
        qDebug() << "Wait for screen refresh failed!";
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
