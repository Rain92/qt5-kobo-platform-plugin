#ifndef DITHER_H
#define DITHER_H

#ifdef __ARM_NEON__
#include <arm_neon.h>
#endif

void ditherBuffer(uint8_t* bufferDst, uint8_t* bufferSrc, int width, int height);
void ditherBufferInplace(uint8_t* buffer, int width, int height);

#endif  // DITHER_H
