#pragma once

#include "CFHDTypes.h"

unsigned int GetRand(unsigned int seed);
void initBaseTransform(void);
void makeVariations(void);
void modifyQBistGenes(void);
void RunQBist(int width, int height, int pitch, CFHD_PixelFormat pixelFormat, int alpha, unsigned char *ptr);
void QBist(float x, float y, unsigned short *r, unsigned short *g, unsigned short *b);