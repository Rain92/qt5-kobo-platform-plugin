#ifndef EINKENUMS_H
#define EINKENUMS_H

#include <QtCore>

enum KoboKey
{
    Key_Light = Qt::Key_BrightnessAdjust,
    Key_Home = Qt::Key_Home,
    Key_Power = Qt::Key_PowerDown,
    Key_SleepCover = Qt::Key_PowerOff + 10000,
    Key_PagePackward = Qt::Key_PageUp,
    Key_PageForward = Qt::Key_PageDown,
    Key_Eraser = 0x01010001,
    Key_Highlighter = 0x01010002
};

static const QMap<int, KoboKey> KoboPhysicalKeyMap = {
    {35, Key_SleepCover},   {59, Key_SleepCover}, {90, Key_Light},
    {102, Key_Home},        {116, Key_Power},     {193, Key_PagePackward},
    {194, Key_PageForward}, {331, Key_Eraser},    {332, Key_Highlighter},
};

enum ScreenRotation
{
    RotationUR = 0,
    RotationCW = 1,
    RotationUD = 2,
    RotationCCW = 3
};

enum WaveForm
{
    WaveForm_AUTO = 257,
    // Let the EPDC choose, via histogram analysis of the refresh region.
    // May *not* always (or ever) opt to use REAGL on devices where it is otherwise
    // available. This is the default. If you request a flashing update w/ AUTO, FBInk
    // automatically uses GC16 instead.
    // Common
    WaveForm_DU = 1,
    // From any to B&W, fast (~260ms), some light ghosting.
    // On-screen pixels will be left as-is for new content that is *not* B&W.
    // Great for UI highlights, or tracing touch/pen input.
    // Will never flash.
    // DU stands for "Direct Update".
    WaveForm_GC16 = 2,
    // From any to any, ~450ms, high fidelity (i.e., lowest risk of ghosting).
    // Ideal for image content.
    // If flashing, will flash and update the full region.
    // If not, only changed pixels will update.
    // GC stands for "Grayscale Clearing"
    WaveForm_GC4 = 3,
    // From any to B/W/GRAYA/GRAY5, (~290ms), some ghosting. (may be implemented as DU4 on some devices).
    // Will *probably* never flash, especially if the device doesn't implement any other 4 color
    // modes. Limited use-cases in practice.
    WaveForm_A2 = 4,
    // From B&W to B&W, fast (~120ms), some ghosting.
    // On-screen pixels will be left as-is for new content that is *not* B&W.
    // FBInk will ask the EPDC to enforce quantization to B&W to honor the "to" requirement,
    // (via EPDC_FLAG_FORCE_MONOCHROME).
    // Will never flash.
    // Consider bracketing a series of A2 refreshes between white screens to transition in and out
    // of A2, so as to honor the "from" requirement, (especially given that FORCE_MONOCHROME may
    // not be reliably able to do so, c.f., refresh_kobo_mk7): non-flashing GC16 for the in
    // transition, A2 or GC16 for the out transition. A stands for "Animation"
    WaveForm_GL16 = 5,
    // From white to any, ~450ms, some ghosting.
    // Typically optimized for text on a white background.
    // Newer generation devices only
    WaveForm_REAGL = 6,
    // From white to any, ~450ms, with ghosting and flashing reduction.
    // When available, best option for text (in place of GL16).
    // May enforce timing constraints if in collision with another waveform mode, e.g.,
    // it may, to some extent, wait for completion of previous updates to have access to HW
    // resources. Marketing term for the feature is "Regal". Technically called 5-bit waveform
    // modes.
    WaveForm_REAGLD = 7
    // From white to any, ~450ms, with more ghosting reduction, but less flashing reduction.
    // Should only be used when flashing, which should yield a less noticeable flash than GC16.
    // Rarely used in practice, because still optimized for text or lightly mixed content,
    // not pure image content.
};

#endif  // EINKENUMS_H
