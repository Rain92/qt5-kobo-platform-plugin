#include "dither.h"

#define SIMD_NEON_PREFECH_SIZE 384

static inline uint32_t div255(uint32_t v)
{
    uint32_t _v = v + 128;
    return ((_v >> 8U) + _v) >> 8U;
}

static inline uint8_t dither_o8x8(unsigned short int x, unsigned short int y, uint8_t v)
{
    // c.f.,
    // https://github.com/ImageMagick/ImageMagick/blob/ecfeac404e75f304004f0566557848c53030bad6/config/thresholds.xml#L107
    static const uint8_t threshold_map_o8x8[] = {
        1,  49, 13, 61, 4,  52, 16, 64, 33, 17, 45, 29, 36, 20, 48, 32, 9,  57, 5,  53, 12, 60,
        8,  56, 41, 25, 37, 21, 44, 28, 40, 24, 3,  51, 15, 63, 2,  50, 14, 62, 35, 19, 47, 31,
        34, 18, 46, 30, 11, 59, 7,  55, 10, 58, 6,  54, 43, 27, 39, 23, 42, 26, 38, 22};

    // Constants:
    // Quantum = 8; Levels = 16; map Divisor = 65
    // QuantumRange = 0xFF
    // QuantumScale = 1.0 / QuantumRange
    //
    // threshold = QuantumScale * v * ((L-1) * (D-1) + 1)
    // NOTE: The initial computation of t (specifically, what we pass to DIV255) would overflow an uint8_t.
    //       With a Q8 input value, we're at no risk of ever underflowing, so, keep to unsigned maths.
    //       Technically, an uint16_t would be wide enough, but it gains us nothing,
    //       and requires a few explicit casts to make GCC happy ;).
    uint32_t t = div255(v * ((15U << 6U) + 1U));
    // level = t / (D-1);
    const uint32_t l = (t >> 6U);
    // t -= l * (D-1);
    t = (t - (l << 6U));

    // map width & height = 8
    // c = ClampToQuantum((l+(t >= map[(x % mw) + mw * (y % mh)])) * QuantumRange / (L-1));
    const uint32_t q = ((l + (t >= threshold_map_o8x8[(x & 7U) + 8U * (y & 7U)])) * 17U);
    // NOTE: We're doing unsigned maths, so, clamping is basically MIN(q, UINT8_MAX) ;).
    //       The only overflow we should ever catch should be for a few white (v = 0xFF) input pixels
    //       that get shifted to the next step (i.e., q = 272 (0xFF + 17)).
    return (q > UINT8_MAX ? UINT8_MAX : (uint8_t)q);
}

#ifdef __ARM_NEON__

static inline uint16x4_t vdiv255(uint32x4_t vec)
{
    uint32x4_t vc128 = vdupq_n_u32(128);
    uint32x4_t vec_v = vaddq_u32(vec, vc128);
    uint32x4_t res = vshrq_n_u32(vaddq_u32(vshrq_n_u32(vec_v, 8), vec_v), 8);
    uint16x4_t res_downcast = vmovn_u32(res);
    return res_downcast;
}

void dither_NEON(uint8_t* bufferDest, uint8_t* bufferSrc, int width, int height)
{
    static const uint8_t threshold_map_o8x8[] = {
        1,  49, 13, 61, 4,  52, 16, 64, 33, 17, 45, 29, 36, 20, 48, 32, 9,  57, 5,  53, 12, 60,
        8,  56, 41, 25, 37, 21, 44, 28, 40, 24, 3,  51, 15, 63, 2,  50, 14, 62, 35, 19, 47, 31,
        34, 18, 46, 30, 11, 59, 7,  55, 10, 58, 6,  54, 43, 27, 39, 23, 42, 26, 38, 22};

    int x = 0, y = 0, cp = 0;

    int size = width * height;

    if (width >= 8)
    {
        uint16_t cx = ((15U << 6) + 1U);

        uint16x8_t vc1 = vdupq_n_u16(1);
        uint16x8_t vc255 = vdupq_n_u16(255);

        uint16x4_t vcx = vdup_n_u16(cx);

        __builtin_prefetch(bufferSrc + SIMD_NEON_PREFECH_SIZE);
        uint8x8_t vecn = vld1_u8(bufferSrc);

        int line_leftover = (8 - width) & 7;  // (8 - width % 8) % 8
        for (y = 0, cp = 0; y < height; y++, cp -= line_leftover)
        {
            for (x = 0; x < width && cp + 8 <= size; x += 8, cp += 8)
            {
                // uint8x8_t vecn = vld1_u8((uchar*)buffer.data() + cp);
                uint16x8_t vec = vmovl_u8(vecn);
                uint16x4_t vec_1 = vget_low_u16(vec);
                uint16x4_t vec_2 = vget_high_u16(vec);

                uint32x4_t vect0_1 = vmull_u16(vec_1, vcx);
                uint32x4_t vect0_2 = vmull_u16(vec_2, vcx);

                uint16x4_t vect_1 = vdiv255(vect0_1);
                uint16x4_t vect_2 = vdiv255(vect0_2);

                uint16x8_t vect = vcombine_u16(vect_1, vect_2);

                uint16x8_t vecl = vshrq_n_u16(vect, 6);
                vect = vsubq_u16(vect, vshlq_n_u16(vecl, 6));

                uint8x8_t vecthreshn = vld1_u8(&threshold_map_o8x8[(x & 7U) + 8U * (y & 7U)]);
                uint16x8_t vecthresh = vmovl_u8(vecthreshn);

                uint16x8_t vecm = vcgeq_u16(vect, vecthresh);

                uint16x8_t vecl2 = vaddq_u16(vecl, vc1);

                uint16x8_t vecq = vbslq_u16(vecm, vecl2, vecl);
                vecq = vmulq_n_u16(vecq, 17);
                vecq = vminq_u16(vecq, vc255);

                uint8x8_t vecqb = vmovn_u16(vecq);

                // load next chunk before writing back
                if (cp + 16 <= size)
                {
                    int offset = cp + 8;
                    if (x + 8 >= width)
                        offset -= line_leftover;

                    __builtin_prefetch(bufferSrc + offset + SIMD_NEON_PREFECH_SIZE);
                    vecn = vld1_u8(bufferSrc + offset);
                }

                vst1_u8(bufferDest + cp, vecqb);
            }
        }

        // wind back loop increments
        cp -= 8 - line_leftover;
        x -= 8;
        y -= 1;
    }

    // take care of leftovers
    for (; y < height; y++)
        for (; x < width; x++, cp++)
            bufferDest[cp] = dither_o8x8(x, y, bufferSrc[cp]);
}

#endif

void dither_fallback(uint8_t* bufferDst, uint8_t* bufferSrc, int width, int height)
{
    for (int y = 0, p = 0; y < height; y++)
        for (int x = 0; x < width; x++, p++)
            bufferDst[p] = dither_o8x8(x, y, bufferSrc[p]);
}

void ditherBuffer(uint8_t* bufferDest, uint8_t* bufferSrc, int width, int height)
{
#ifdef __ARM_NEON__
    dither_NEON(bufferDest, bufferSrc, width, height);
#else
    dither_fallback(bufferDest, bufferSrc, width, height);
#endif
}

void ditherBufferInplace(uint8_t* buffer, int width, int height)
{
#ifdef __ARM_NEON__
    dither_NEON(buffer, buffer, width, height);
#else
    dither_fallback(buffer, buffer, width, height);
#endif
}

const uint8_t VALUES_12BPP[] = {0, 17, 34, 51, 68, 85, 102, 119, 136, 153, 170, 187, 204, 221, 238, 255};

#define MIN(a, b) (((a) < (b)) ? (a) : (b))
#define MAX(a, b) (((a) > (b)) ? (a) : (b))
#define CLAMPED(x, xmin, xmax) MAX((xmin), MIN((xmax), (x)))

//	Floyd-Steinberg dither uses constants 7/16 5/16 3/16 and 1/16
//	But instead of using real arythmetic, I will use integer on by
//	applying shifting ( << 8 )
//	When use the constants, don't foget to shift back the result ( >> 8 )
#define f7_16 112  // const int	f7	= (7 << 8) / 16;
#define f5_16 80   // const int	f5	= (5 << 8) / 16;
#define f3_16 48   // const int	f3	= (3 << 8) / 16;
#define f1_16 16   // const int	f1	= (1 << 8) / 16;

//	Color Floyd-Steinberg dither using 4 bit per color plane
void ditherFloydSteinberg_(uint8_t* dest, uint8_t* src, int width, int height)
{
    const int size = width * height;

    if (src != dest)
        std::memcpy(dest, src, size);

    for (int y = 0, i = 0; y < height; y++)
    {
        for (int x = 0; x < width; x++, i++)
        {
            const uint8_t oldVal = dest[i];

            int newVal = VALUES_12BPP[oldVal >> 4];

            dest[i] = newVal;

            int cerrorB = oldVal - newVal;

            int errorIdx = i + 1;
            if (x + 1 < width)
                dest[i + 1] = CLAMPED(dest[errorIdx] + ((cerrorB * f7_16) >> 8), 0, 255);

            errorIdx += width - 2;
            if (x - 1 > 0 && y + 1 < height)
                dest[i + width - 1] = CLAMPED(dest[errorIdx] + ((cerrorB * f3_16) >> 8), 0, 255);

            errorIdx++;
            if (y + 1 < height)
                dest[i + width + 0] = CLAMPED(dest[errorIdx] + ((cerrorB * f5_16) >> 8), 0, 255);

            errorIdx++;
            if (x + 1 < width && y + 1 < height)
                dest[i + width + 1] = CLAMPED(dest[errorIdx] + ((cerrorB * f1_16) >> 8), 0, 255);
        }
    }
}

//	Color Floyd-Steinberg dither using 4 bit per color plane
void ditherFloydSteinberg(uint8_t* dest, uint8_t* src, int width, int height)
{
    const int size = width * height;

    int16_t* errorB = (int16_t*)malloc(size * sizeof(int16_t));
    memset(errorB, 0, size * sizeof(int16_t));

    for (int y = 0, offset = 0, i = 0; y < height; y++, offset += width)
    {
        for (int x = 0; x < width; x++, i++)
        {
            const uint8_t oldVal = src[offset + x];

            int oldValErr = oldVal + (errorB[i] >> 8);

            //	The error could produce values beyond the borders, so need to keep the color in range
            int idxB = CLAMPED(oldVal + (errorB[i] >> 8), 0, 255);

            int newVal = VALUES_12BPP[idxB >> 4];  //	x >> 4 is the same as x / 16

            dest[offset + x] = newVal;

            int quantError = oldValErr - newVal;

            if (x + 1 < width)
                errorB[i + 1] += (quantError * f7_16);

            if (x - 1 > 0 && y + 1 < height)
                errorB[i + width - 1] += (quantError * f3_16);

            if (y + 1 < height)
                errorB[i + width + 0] += (quantError * f5_16);

            if (x + 1 < width && y + 1 < height)
                errorB[i + width + 1] += (quantError * f1_16);
        }
    }

    free(errorB);
}

//	Color Floyd-Steinberg dither using 4 bit per color plane
void ditherFloydSteinbergN(uint8_t* dest, uint8_t* src, int width, int height)
{
    const int size = width * height;

    int16_t* errorB = (int16_t*)malloc(size * sizeof(int16_t));
    memset(errorB, 0, size * sizeof(int16_t));

    int16_t* errorLine = (int16_t*)malloc(width * sizeof(int16_t));

    for (int y = 0, offset = 0, i = 0; y < height; y++, offset += width)
    {
        for (int x = 0; x < width; x++, i++)
        {
            const uint8_t oldVal = src[offset + x];

            int oldValErr = oldVal + (errorB[i] >> 8);

            //	The error could produce values beyond the borders, so need to keep the color in range
            int idxB = CLAMPED(oldVal + (errorB[i] >> 8), 0, 255);

            int newVal = VALUES_12BPP[idxB >> 4];  //	x >> 4 is the same as x / 16

            dest[offset + x] = newVal;

            int quantError = oldValErr - newVal;
            errorLine[x] = quantError;

            int errorIdx = i + 1;
            if (x + 1 < width)
                errorB[errorIdx] += (quantError * f7_16);
        }
        if (y + 1 < height)
        {
            int x = 0;
            {
                int16_t quantError = errorLine[x];
                // errorB[i + x - 1] += (quantError * f3_16);
                errorB[i + x + 0] += (quantError * f5_16);
                errorB[i + x + 1] += (quantError * f1_16);
            }
            for (x = 1; x + 7 < width - 1; x += 8)
            {
                int16x8_t quantErrorv = vld1q_s16(&errorLine[x]);

                int16x8_t multv1 = vmulq_n_s16(quantErrorv, f3_16);
                int16x8_t resErrorv1 = vld1q_s16(&errorB[i + x - 1]);
                int16x8_t resAddv1 = vaddq_s16(resErrorv1, multv1);
                vst1q_s16(&errorB[i + x - 1], resAddv1);

                int16x8_t multv2 = vmulq_n_s16(quantErrorv, f5_16);
                int16x8_t resErrorv2 = vld1q_s16(&errorB[i + x]);
                int16x8_t resAddv2 = vaddq_s16(resErrorv2, multv2);
                vst1q_s16(&errorB[i + x], resAddv2);

                int16x8_t multv3 = vmulq_n_s16(quantErrorv, f1_16);
                int16x8_t resErrorv3 = vld1q_s16(&errorB[i + x + 1]);
                int16x8_t resAddv3 = vaddq_s16(resErrorv3, multv3);
                vst1q_s16(&errorB[i + x + 1], resAddv3);
            }
            for (; x < width; x++)
            {
                int16_t quantError = errorLine[x];
                errorB[i + x - 1] += (quantError * f3_16);
                errorB[i + x + 0] += (quantError * f5_16);
                if (x + 1 < width)
                    errorB[i + x + 1] += (quantError * f1_16);
            }
        }
    }

    free(errorB);
    free(errorLine);
}
