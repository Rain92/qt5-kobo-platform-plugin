#ifndef EINK_SUNXI_H
#define EINK_SUNXI_H

#include <fcntl.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <unistd.h>

#include <QDebug>

#include "ion-kobo.h"
#include "mxcfb-kobo.h"
#include "sunxi-kobo.h"
#include "sys/ioctl.h"

#define ALIGN(x, a)                         \
    (                                       \
        {                                   \
            auto mask__ = (a)-1U;           \
            (((x) + (mask__)) & ~(mask__)); \
        })
#define IS_ALIGNED(x, a)                          \
    (                                             \
        {                                         \
            auto mask__ = (a)-1U;                 \
            ((x) & (mask__)) == 0 ? true : false; \
        })

typedef struct
{
    uint16_t bus;
    uint16_t address;
} FBInkI2CDev;

typedef int8_t SUNXI_FORCE_ROTA_INDEX_T;

// Chuck all the sunxi mess in a single place...
typedef struct
{
    int disp_fd;
    int i2c_fd;
    FBInkI2CDev i2c_dev;
    int ion_fd;
    size_t alloc_size;
    struct ion_fd_data ion;
    struct disp_layer_config2 layer;
    uint32_t rota;
    // NOTE: If we could actually somehow detect Nickel's (and/or the working buffer's) actual screen
    // layout/rotation,
    //       this would be even more useful, because right now it's just a weird kludge...
    SUNXI_FORCE_ROTA_INDEX_T force_rota;
} FBInkKoboSunxi;

void fixupSunxiFB(FBInkKoboSunxi &sunxiCtx, fb_fix_screeninfo &fInfo, fb_var_screeninfo &vInfo);

FBInkKoboSunxi setupIonLayer(fb_var_screeninfo &vInfo);
int memmap_ion(unsigned char *&fbPtr, FBInkKoboSunxi &sunxiCtx, fb_fix_screeninfo &fInfo);
int unmap_ion(unsigned char *&fbPtr, FBInkKoboSunxi &sunxiCtx);

int refreshScreenRegionSunxi(FBInkKoboSunxi &sunxiCtx, const struct mxcfb_rect region, uint32_t waveform_mode,
                             uint32_t update_mode, int dithering_mode, uint32_t &marker);
int waitRefreshCompleteSunxi(FBInkKoboSunxi &sunxiCtx, uint32_t marker);

#endif  // EINK_SUNXI_H
