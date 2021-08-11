#ifndef EINK_H
#define EINK_H

#include <stdlib.h>

#include <QDebug>

#include "mxcfb-kobo.h"
#include "sys/ioctl.h"

int refreshScreenRegion(int fbfd, const struct mxcfb_rect region, uint32_t waveform_mode,
                        uint32_t update_mode, uint32_t marker);

int waitRefreshComplete(int fbfd, uint32_t marker);

int refreshScreenRegionMk7(int fbfd, const struct mxcfb_rect region, uint32_t waveform_mode,
                           uint32_t update_mode, int dithering_mode, uint32_t marker);
int waitRefreshCompleteMk7(int fbfd, uint32_t marker);


#endif  // EINK_H
