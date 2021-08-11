#include "eink_sunxi.h"

void fixupSunxiFB(FBInkKoboSunxi &sunxiCtx, fb_fix_screeninfo &fInfo, fb_var_screeninfo &vInfo)
{
    vInfo.rotate = 3;

    sunxiCtx.rota = ((3 - vInfo.rotate) & 3) * 90U;

    const uint32_t xres = vInfo.xres;
    const uint32_t yres = vInfo.yres;
    if ((vInfo.rotate & 0x01) == 1)
    {
        // Odd, Landscape
        vInfo.xres = std::max(xres, yres);
        vInfo.yres = std::min(xres, yres);
    }
    else
    {
        // Even, Portrait
        vInfo.xres = std::min(xres, yres);
        vInfo.yres = std::max(xres, yres);
    }

    vInfo.xres_virtual = vInfo.xres;
    vInfo.yres_virtual = vInfo.yres;

    // Make it grayscale...
    vInfo.bits_per_pixel = 8U;
    vInfo.grayscale = 1U;

    fInfo.line_length = (vInfo.xres_virtual * vInfo.bits_per_pixel) >> 3U;

    // Used by clear_screen & memmap_ion
    fInfo.smem_len = fInfo.line_length * vInfo.yres_virtual;
}

FBInkKoboSunxi setupIonLayer(fb_var_screeninfo &vInfo)
{
    FBInkKoboSunxi sunxiCtx = {.disp_fd = -1,
                               .i2c_fd = -1,
                               .i2c_dev = {0},
                               .ion_fd = -1,
                               .alloc_size = 0U,
                               .ion = {.handle = 0, .fd = -1},
                               //                           .layer = {{0}},
                               .rota = 0U,
                               .force_rota = 0};

    // disp_layer_info2
    sunxiCtx.layer.info.mode = LAYER_MODE_BUFFER;
    sunxiCtx.layer.info.zorder = 0U;
    // NOTE: Ignore pixel alpha.
    //       We actually *do* handle alpha sanely, so,
    //       if we were actually using an RGB32 fb, we might want to tweak that & pre_multiply...
    sunxiCtx.layer.info.alpha_mode = 1U;
    sunxiCtx.layer.info.alpha_value = 0xFFu;

    // disp_rect
    sunxiCtx.layer.info.screen_win.x = 0;
    sunxiCtx.layer.info.screen_win.y = 0;
    sunxiCtx.layer.info.screen_win.width = vInfo.xres;
    sunxiCtx.layer.info.screen_win.height = vInfo.yres;

    sunxiCtx.layer.info.b_trd_out = false;
    sunxiCtx.layer.info.out_trd_mode = DISP_3D_OUT_MODE_TB;

    // disp_fb_info2
    // NOTE: fd & y8_fd are handled in mmap_ion.
    //       And they are *explicitly* set to 0 and not -1 when unused,
    //       because that's what the disp code (mostly) expects (*sigh*).

    // disp_rectsz
    // NOTE: Used in conjunction with align below.
    //       We obviously only have a single buffer, because we're not a 3D display...
    sunxiCtx.layer.info.fb.size[0].width = vInfo.xres_virtual;
    sunxiCtx.layer.info.fb.size[0].height = vInfo.yres_virtual;
    sunxiCtx.layer.info.fb.size[1].width = 0U;
    sunxiCtx.layer.info.fb.size[1].height = 0U;
    sunxiCtx.layer.info.fb.size[2].width = 0U;
    sunxiCtx.layer.info.fb.size[2].height = 0U;

    // NOTE: Used to compute the scanline pitch in bytes (e.g., pitch = ALIGN(scanline_pixels * components,
    // align).
    //       This is set to 2 by Nickel, but we appear to go by without it just fine with a Y8 fb fd...
    sunxiCtx.layer.info.fb.align[0] = 0U;
    sunxiCtx.layer.info.fb.align[1] = 0U;
    sunxiCtx.layer.info.fb.align[2] = 0U;
    sunxiCtx.layer.info.fb.format = DISP_FORMAT_8BIT_GRAY;
    sunxiCtx.layer.info.fb.color_space = DISP_GBR_F;  // Full-range RGB
    sunxiCtx.layer.info.fb.trd_right_fd = 0;
    sunxiCtx.layer.info.fb.pre_multiply = true;  // Because we're using global alpha, I guess?
    sunxiCtx.layer.info.fb.crop.x = 0;
    sunxiCtx.layer.info.fb.crop.y = 0;
    // Don't ask me why this needs to be shifted 32 bits to the left... ¯\_(ツ)_/¯
    // NOTE: I managed to bork it during the KOReader port and it appeared to behave fine *without* the
    // shift...
    sunxiCtx.layer.info.fb.crop.width = (uint64_t)vInfo.xres << 32U;
    sunxiCtx.layer.info.fb.crop.height = (uint64_t)vInfo.yres << 32U;
    sunxiCtx.layer.info.fb.flags = DISP_BF_NORMAL;
    sunxiCtx.layer.info.fb.scan = DISP_SCAN_PROGRESSIVE;
    sunxiCtx.layer.info.fb.eotf = DISP_EOTF_GAMMA22;  // SDR
    sunxiCtx.layer.info.fb.depth = 0;
    sunxiCtx.layer.info.fb.fbd_en = 0U;
    sunxiCtx.layer.info.fb.metadata_fd = 0;
    sunxiCtx.layer.info.fb.metadata_size = 0U;
    sunxiCtx.layer.info.fb.metadata_flag = 0U;

    sunxiCtx.layer.info.id = 0U;

    // disp_atw_info
    sunxiCtx.layer.info.atw.used = false;
    sunxiCtx.layer.info.atw.mode = NORMAL_MODE;
    sunxiCtx.layer.info.atw.b_row = 0;
    sunxiCtx.layer.info.atw.b_col = 0;
    sunxiCtx.layer.info.atw.cof_fd = 0;

    sunxiCtx.layer.enable = true;
    sunxiCtx.layer.channel = 0U;
    // NOTE: Nickel uses layer 0, pickel layer 1.
    sunxiCtx.layer.layer_id = 1U;

    return sunxiCtx;
}

int memmap_ion(unsigned char *&fbPtr, FBInkKoboSunxi &sunxiCtx, fb_fix_screeninfo &fInfo)
{
    int rv = EXIT_SUCCESS;
    bool isFbMapped = false;

    // Start by registering as an ION client
    sunxiCtx.ion_fd = open("/dev/ion", O_RDONLY | O_NONBLOCK | O_CLOEXEC);
    if (sunxiCtx.ion_fd == -1)
    {
        qDebug() << "opening /dev/ion failed";
        printf("open: %m");
        return EXIT_FAILURE;
    }

    // Then request a page-aligned carveout mapping large enough to fit our screen.
    sunxiCtx.alloc_size = ALIGN(fInfo.smem_len, 4096);
    struct ion_allocation_data alloc = {
        .len = sunxiCtx.alloc_size, .align = 4096, .heap_id_mask = ION_HEAP_MASK_CARVEOUT};
    int ret = ioctl(sunxiCtx.ion_fd, ION_IOC_ALLOC, &alloc);
    if (ret < 0)
    {
        qDebug() << "alloc failed!";
        printf("ION_IOC_ALLOC: %m");
        rv = EXIT_FAILURE;
        goto cleanup;
    }

    // Request a dmabuff handle that we can share & mmap for that alloc
    sunxiCtx.ion.handle = alloc.handle;
    ret = ioctl(sunxiCtx.ion_fd, ION_IOC_MAP, &sunxiCtx.ion);
    if (ret < 0)
    {
        qDebug() << "ioctl(sunxiCtx.ion_fd, ION_IOC_MAP, &sunxiCtx.ion) failed!";
        printf("ION_IOC_MAP: %m");
        rv = EXIT_FAILURE;
        goto cleanup;
    }

    // Finally, mmap it...
    fbPtr = (unsigned char *)mmap(NULL, sunxiCtx.alloc_size, PROT_READ | PROT_WRITE, MAP_SHARED,
                                  sunxiCtx.ion.fd, 0);
    if (fbPtr == MAP_FAILED)
    {
        qDebug() << "mmap failed!";
        printf("mmap: %m");
        fbPtr = NULL;
        rv = EXIT_FAILURE;
        goto cleanup;
    }
    else
    {
        isFbMapped = true;
    }

    // And finally, register as a DISP client, too
    sunxiCtx.disp_fd = open("/dev/disp", O_RDONLY | O_NONBLOCK | O_CLOEXEC);
    if (sunxiCtx.disp_fd == -1)
    {
        qDebug() << "Failed to open /dev/disp";
        printf("open: %m");
        rv = EXIT_FAILURE;
        goto cleanup;
    }

    // And update our layer config to use that dmabuff fd, as a grayscale buffer.
    sunxiCtx.layer.info.fb.fd = 0;
    sunxiCtx.layer.info.fb.y8_fd = sunxiCtx.ion.fd;

    qDebug() << "Ion mmap succeeded:" << sunxiCtx.ion_fd << sunxiCtx.ion.handle << sunxiCtx.alloc_size
             << fbPtr << isFbMapped;

    return rv;

cleanup:
    if (isFbMapped)
    {
        if (munmap(fbPtr, sunxiCtx.alloc_size) < 0)
        {
            printf("munmap: %m");
            rv = EXIT_FAILURE;
        }
        else
        {
            isFbMapped = false;
            fbPtr = NULL;
            sunxiCtx.alloc_size = 0U;

            if (close(sunxiCtx.ion.fd) != 0)
            {
                printf("close: %m");
                rv = EXIT_FAILURE;
            }
            else
            {
                sunxiCtx.ion.fd = -1;
            }
        }
    }

    if (sunxiCtx.ion.handle != 0)
    {
        struct ion_handle_data handle = {.handle = sunxiCtx.ion.handle};
        ret = ioctl(sunxiCtx.ion_fd, ION_IOC_FREE, &handle);
        if (ret < 0)
        {
            printf("ION_IOC_FREE: %m");
            rv = EXIT_FAILURE;
        }
        else
        {
            sunxiCtx.ion.handle = 0;
        }
    }

    if (sunxiCtx.ion_fd != -1)
    {
        if (close(sunxiCtx.ion_fd) != 0)
        {
            printf("close: %m");
            rv = EXIT_FAILURE;
        }
        else
        {
            sunxiCtx.ion_fd = -1;
        }
    }

    if (sunxiCtx.disp_fd != -1)
    {
        if (close(sunxiCtx.disp_fd) != 0)
        {
            printf("close: %m");
            rv = EXIT_FAILURE;
        }
        else
        {
            sunxiCtx.disp_fd = -1;
        }
    }

    qDebug() << "Ion mmap failed:" << sunxiCtx.ion_fd << sunxiCtx.ion.handle << sunxiCtx.alloc_size
             << isFbMapped;

    return rv;
}

int unmap_ion(unsigned char *&fbPtr, FBInkKoboSunxi &sunxiCtx)
{
    int rv = EXIT_SUCCESS;

    if (munmap(fbPtr, sunxiCtx.alloc_size) < 0)
    {
        printf("munmap: %m");
        rv = EXIT_FAILURE;
    }
    else
    {
        //        isFbMapped = false;
        fbPtr = NULL;
        sunxiCtx.alloc_size = 0U;

        if (close(sunxiCtx.ion.fd) != 0)
        {
            printf("close: %m");
            rv = EXIT_FAILURE;
        }
        else
        {
            sunxiCtx.ion.fd = -1;
        }
    }

    struct ion_handle_data handle = {.handle = sunxiCtx.ion.handle};
    int ret = ioctl(sunxiCtx.ion_fd, ION_IOC_FREE, &handle);
    if (ret < 0)
    {
        printf("ION_IOC_FREE: %m");
        rv = EXIT_FAILURE;
    }
    else
    {
        sunxiCtx.ion.handle = 0;
        sunxiCtx.layer.info.fb.fd = 0;
        sunxiCtx.layer.info.fb.y8_fd = 0;
    }

    if (close(sunxiCtx.ion_fd) != 0)
    {
        printf("close: %m");
        rv = EXIT_FAILURE;
    }
    else
    {
        sunxiCtx.ion_fd = -1;
    }

    if (close(sunxiCtx.disp_fd) != 0)
    {
        printf("close: %m");
        rv = EXIT_FAILURE;
    }
    else
    {
        sunxiCtx.disp_fd = -1;
    }

    return rv;
}

// Kobo Mark 8 devices ([Mk8<->??)
int refreshScreenRegionSunxi(FBInkKoboSunxi &sunxiCtx, const struct mxcfb_rect region, uint32_t waveform_mode,
                             uint32_t update_mode, int dithering_mode, uint32_t &marker)
{
    // NOTE: In case of issues, enable full verbosity in the DISP driver:
    //       echo 8 >| /proc/sys/kernel/printk
    //       echo 9 >| /sys/kernel/debug/dispdbg/dgblvl
    //       And running klogd so you get everything timestamped in the syslog always helps ;).
    //       "Small" caveat: it appears to make the driver *that* much buggy and crashy...
    // NOTE: Speaking of debugfs, Nickel periodically (haven't looked at the circumstances in detail)
    //       wakes up the EPDC via debugfs (e.g., name=lcd0, command=enable, start=1).
    //       Possibly related to the aggressive standby on idle? (Which I haven't looked into there, either).

    // Convert our mxcfb_rect into a sunxi area_info
    struct area_info area = {.x_top = region.left,
                             .y_top = region.top,
                             .x_bottom = region.left + region.width - 1,
                             .y_bottom = region.top + region.height - 1};

    sunxi_disp_eink_update2 update = {.area = &area,
                                      .layer_num = 1U,
                                      .update_mode = waveform_mode,
                                      .lyr_cfg2 = &sunxiCtx.layer,
                                      .frame_id = &marker,
                                      .rotate = &sunxiCtx.rota,
                                      .cfa_use = 0U};

    // Update mode shenanigans...
    if (update_mode == UPDATE_MODE_PARTIAL && waveform_mode != EINK_AUTO_MODE)
    {
        // For some reason, AUTO shouldn't specify RECT...
        // (it trips the unknown mode warning, which falls back to... plain AUTO ;)).
        update.update_mode |= EINK_RECT_MODE;
    }
    if (waveform_mode == EINK_GLR16_MODE || waveform_mode == EINK_GLD16_MODE)
    {
        // NOTE: Fun fact: nothing currently checks this flag in the ELipsa kernel... -_-".
        update.update_mode |= EINK_REGAL_MODE;
    }
    if (waveform_mode == EINK_A2_MODE)
    {
        // NOTE: Unlike on mxcfb, this isn't HW assisted, this just uses the "simple" Y8->Y1 dither
        // algorithm...
        update.update_mode |= EINK_MONOCHROME;
    }

    int rv = ioctl(sunxiCtx.disp_fd, DISP_EINK_UPDATE2, &update);

    qDebug() << "update marker" << marker;

    if (rv < 0)
    {
        qDebug() << "update region failed" << sunxiCtx.layer.info.screen_win.x
                 << sunxiCtx.layer.info.screen_win.y << sunxiCtx.layer.info.screen_win.width
                 << sunxiCtx.layer.info.screen_win.height << region.top << region.left << region.width
                 << region.height;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}

int waitRefreshCompleteSunxi(FBInkKoboSunxi &sunxiCtx, uint32_t marker)
{
    // Use the union to avoid passing garbage to the ioctl handler...
    sunxi_disp_eink_ioctl cmd = {{0}};
    cmd.wait_for.frame_id = marker;

    int rv = ioctl(sunxiCtx.disp_fd, DISP_EINK_WAIT_FRAME_SYNC_COMPLETE, &cmd);

    if (rv < 0)
    {
        qDebug() << "Wait for screen refresh failed!";
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
