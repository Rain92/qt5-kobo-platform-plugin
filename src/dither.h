#ifndef DITHER_H
#define DITHER_H

#include <stdint.h>
#include <stdlib.h>

#include <cstring>

#ifdef __ARM_NEON__
#include <arm_neon.h>
#endif

void ditherBuffer(uint8_t* bufferDest, uint8_t* bufferSrc, int width, int height);
void ditherBufferInplace(uint8_t* buffer, int width, int height);

void ditherFloydSteinberg(uint8_t* dest, uint8_t* src, int width, int height);
void ditherFloydSteinbergN(uint8_t* dest, uint8_t* src, int width, int height);

#endif  // DITHER_H
