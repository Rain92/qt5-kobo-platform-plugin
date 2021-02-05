#ifndef EINK_H
#define EINK_H

#include <stdlib.h>

#include <QDebug>

#include "mxcfb-kobo.h"
#include "sys/ioctl.h"

int refreshScreenRegion(int fbfd, const struct mxcfb_rect region, uint32_t waveform_mode,
                        uint32_t update_mode, uint32_t marker, uint32_t flags = 0);

int waitRefreshComplete(int fbfd, uint32_t marker);

#endif  // EINK_H
