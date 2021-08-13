#ifndef REFRESHMODE_H
#define REFRESHMODE_H

#define WAVEFORM_MODE_AUTO 257

#define NTX_WFM_MODE_DU 1
#define NTX_WFM_MODE_GC16 2
#define NTX_WFM_MODE_GC4 3
#define NTX_WFM_MODE_A2 4
#define NTX_WFM_MODE_GL16 5
#define NTX_WFM_MODE_GLR16 6
#define NTX_WFM_MODE_GLD16 7

enum PartialRefreshMode
{
    AccuratePartialRefresh = 0,
    FastPartialRefresh,
    MixedPartialRefresh
};

enum WaveForm
{
    WaveForm_AUTO = WAVEFORM_MODE_AUTO,
    // Let the EPDC choose, via histogram analysis of the refresh region.
    // May *not* always (or ever) opt to use REAGL on devices where it is otherwise
    // available. This is the default. If you request a flashing update w/ AUTO, FBInk
    // automatically uses GC16 instead.
    // Common
    WaveForm_DU = NTX_WFM_MODE_DU,
    // From any to B&W, fast (~260ms), some light ghosting.
    // On-screen pixels will be left as-is for new content that is *not* B&W.
    // Great for UI highlights, or tracing touch/pen input.
    // Will never flash.
    // DU stands for "Direct Update".
    WaveForm_GC16 = NTX_WFM_MODE_GC16,
    // From any to any, ~450ms, high fidelity (i.e., lowest risk of ghosting).
    // Ideal for image content.
    // If flashing, will flash and update the full region.
    // If not, only changed pixels will update.
    // GC stands for "Grayscale Clearing"
    WaveForm_GC4 = NTX_WFM_MODE_GC4,
    // From any to B/W/GRAYA/GRAY5, (~290ms), some ghosting. (may be implemented as DU4 on some devices).
    // Will *probably* never flash, especially if the device doesn't implement any other 4 color
    // modes. Limited use-cases in practice.
    WaveForm_A2 = NTX_WFM_MODE_A2,
    // From B&W to B&W, fast (~120ms), some ghosting.
    // On-screen pixels will be left as-is for new content that is *not* B&W.
    // FBInk will ask the EPDC to enforce quantization to B&W to honor the "to" requirement,
    // (via EPDC_FLAG_FORCE_MONOCHROME).
    // Will never flash.
    // Consider bracketing a series of A2 refreshes between white screens to transition in and out
    // of A2, so as to honor the "from" requirement, (especially given that FORCE_MONOCHROME may
    // not be reliably able to do so, c.f., refresh_kobo_mk7): non-flashing GC16 for the in
    // transition, A2 or GC16 for the out transition. A stands for "Animation"
    WaveForm_GL16 = NTX_WFM_MODE_GL16,
    // From white to any, ~450ms, some ghosting.
    // Typically optimized for text on a white background.
    // Newer generation devices only
    WaveForm_REAGL = NTX_WFM_MODE_GLR16,
    // From white to any, ~450ms, with ghosting and flashing reduction.
    // When available, best option for text (in place of GL16).
    // May enforce timing constraints if in collision with another waveform mode, e.g.,
    // it may, to some extent, wait for completion of previous updates to have access to HW
    // resources. Marketing term for the feature is "Regal". Technically called 5-bit waveform
    // modes.
    WaveForm_REAGLD = NTX_WFM_MODE_GLD16
    // From white to any, ~450ms, with more ghosting reduction, but less flashing reduction.
    // Should only be used when flashing, which should yield a less noticeable flash than GC16.
    // Rarely used in practice, because still optimized for text or lightly mixed content,
    // not pure image content.
};

#endif  // REFRESHMODE_H
