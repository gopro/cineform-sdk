/*! @file utils.c

*  @brief wavelet demo
*
*  @version 1.0.0
*
*  (C) Copyright 2017 GoPro Inc (http://gopro.com/).
*
*  Licensed under either:
*  - Apache License, Version 2.0, http://www.apache.org/licenses/LICENSE-2.0  
*  - MIT license, http://opensource.org/licenses/MIT
*  at your option.
*
*  Unless required by applicable law or agreed to in writing, software
*  distributed under the License is distributed on an "AS IS" BASIS,
*  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*  See the License for the specific language governing permissions and
*  limitations under the License.
*
*/
#include <stdio.h>  //need to remove these dependencies for the embedded decoder.
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "utils.h"



void QuantValue(int *src, int w, int x, int y, int multiplier, int midpoint, quantizer_stats *s)
{
	int val, valq;
	val = src[y*w + x];

	valq = (abs(val) + midpoint) * multiplier;
	valq >>= 16;
	if (valq)
		valq *= val / abs(val);

	if (valq)
	{
		int sign = valq / abs(valq);
		valq = abs(valq);
		if (valq > 1023)
		{
			if (s) s->overflow++;   //stats
			valq = 1024;
		}
		valq = s->nonlinearquant[valq] * sign;
	}


	//stats
	if (s)
	{
		if (s->min > val) s->min = val;
		if (s->max < val) s->max = val;
		if (s->minq > valq) s->minq = valq;
		if (s->maxq < valq) s->maxq = valq;

		if (valq > 255)
		{
			s->overflow++;   //stats
			valq = 255;
		}
		if (valq < -255)
		{
			s->overflow++;
			valq = -255;
		}

		s->total++;
		s->energy += abs(valq);
	}

	src[y*w + x] = valq;
}

void QuantizeHighpass(int *src, int w, int h, int regw, int regh, int qh, int qv, int qd, quantizer_stats *s)
{
	int x, y;
	int multiplier;
	int midpoint;

	s->energy = 0;
	s->min = 0;
	s->max = 0;
	s->minq = 0;
	s->maxq = 0;
	s->total = 0;
	s->overflow = 0;

	// Horizontal High Pass
	multiplier = (1 << 16) / qh;
	midpoint = (qh >> 1) - 1;
	if (midpoint < 0) midpoint = 0;
	for (y = 0; y<regh; y++)
	{
		for (x = regw; x<regw * 2; x++)
		{
			QuantValue(src, w, x, y, multiplier, midpoint, s);
		}
	}

	//Vertical High Pass
	multiplier = (1 << 16) / qv;
	midpoint = (qv >> 1) - 1;
	if (midpoint < 0) midpoint = 0;
	for (y = regh; y<regh * 2; y++)
	{
		for (x = 0; x<regw; x++)
		{
			QuantValue(src, w, x, y, multiplier, midpoint, s);
		}
	}

	// Diagonal -  Horizontal & Vertical High Pass
	multiplier = (1 << 16) / qd;
	midpoint = (qd >> 1) - 1;
	if (midpoint < 0) midpoint = 0;
	for (y = regh; y<regh * 2; y++)
	{
		for (x = regw; x<regw * 2; x++)
		{
			QuantValue(src, w, x, y, multiplier, midpoint, s);
		}
	}
//#if SUBBANDS_STATS
	if (s)
		printf("High pass (%d,%d) min,max = %6d,%6d, minq,maxq = %4d,%4d, overflow %5.3f%%, energy = %6d\n", regw, regh, s->min, s->max, s->minq, s->maxq, (float)s->overflow*100.0 / (float)s->total, s->energy);
//#endif
}


void InvQuantValue(int *src, int w, int x, int y, int q, quantizer_stats *s)
{
	int val = src[y*w + x];
	//int sign = 0;

//#if QUANTIZE_NONLINEAR
	if (val)
	{
		int sign = val / abs(val);
		val = abs(val);

		val = s->invnonlinearquant[val];
		val *= sign;
	}
//#endif


//#if QUANTIZE_HIGHPASS
	val *= q;
//#endif

	src[y*w + x] = val;
}

void InverseQuantizeHighpass(int *src, int w, int h, int regw, int regh, int qh, int qv, int qd, quantizer_stats *s)
{
	int x, y;

	// Horizontal High Pass
	for (y = 0; y<regh; y++)
	{
		for (x = regw; x<regw * 2; x++)
		{
			InvQuantValue(src, w, x, y, qh, s);
		}
	}

	//Vertical High Pass
	for (y = regh; y<regh * 2; y++)
	{
		for (x = 0; x<regw; x++)
		{
			InvQuantValue(src, w, x, y, qv, s);
		}
	}

	// Diagonal -  Horizontal & Vertical High Pass
	for (y = regh; y<regh * 2; y++)
	{
		for (x = regw; x<regw * 2; x++)
		{
			InvQuantValue(src, w, x, y, qd, s);
		}
	}
}



void OffsetBuffer(int *src, int w, int h, int regw, int regh, int offset)
{
	int x, y;

	for (y = 0; y<regh; y++)
	{
		for (x = 0; x<regw; x++)
		{
			int val = src[y*w + x];

			val += offset;

			src[y*w + x] = val;
		}
	}
}


void V22Wavelet(int *src, int *dest, int w, int h, int regw, int regh)
{
	int x, y;
	int half = w*(regh / 2);

	for (y = 0; y<regh; y += 2)
	{
		for (x = 0; x<regw; x++)
		{
			dest[(y >> 1)*w + x] = src[y*w + x] + src[(y + 1)*w + x];
#if FIT8BIT
			dest[(y >> 1)*w + x] >>= 1;
#endif

			dest[(y >> 1)*w + x + half] = src[y*w + x] - src[(y + 1)*w + x];
		}
	}
}

void InvertV22Wavelet(int *src, int *dest, int w, int h, int regw, int regh)
{
	int x, y;
	int half = w*regh;

	for (y = 0; y<regh; y++)
	{
		for (x = 0; x<regw * 2; x++)
		{
#if FIT8BIT
			dest[y * 2 * w + x] = (src[y*w + x] * 2 + (src[y*w + x + half])) >> 1;
			dest[(y * 2 + 1)*w + x] = (src[y*w + x] * 2 - (src[y*w + x + half]) >> 1;
#else
			dest[y * 2 * w + x] = (src[y*w + x] + (src[y*w + x + half])) >> 1;
			dest[(y * 2 + 1)*w + x] = (src[y*w + x] - (src[y*w + x + half])) >> 1;
#endif
		}
	}
}

#define ROUNDING			(4)	  

void H26Wavelet(int *src, int *dest, int w, int h, int regw, int regh)
{
	int x, y;//, val;
	int half = regw / 2;

	for (y = 0; y<regh; y++)
	{

		for (x = 0; x<regw; x += 2)
		{
			if (x == 0)
			{
				dest[y*w + (x >> 1)] = (src[y*w + x] + src[y*w + x + 1]);
				dest[y*w + half + (x >> 1)] = ((5 * src[y*w + x] - 11 * src[y*w + x + 1]
					+ 4 * src[y*w + x + 2] + 4 * src[y*w + x + 3]
					- 1 * src[y*w + x + 4] - 1 * src[y*w + x + 5] + ROUNDING) >> 3);
			}
			else if (x<regw - 2)
			{
				dest[y*w + (x >> 1)] = (src[y*w + x] + src[y*w + x + 1]);
				dest[y*w + half + (x >> 1)] = ((-src[y*w + x - 2] - src[y*w + x - 1] + src[y*w + x + 2] + src[y*w + x + 3] + ROUNDING) >> 3) + src[y*w + x] - src[y*w + x + 1];
			}
			else
			{
				dest[y*w + (x >> 1)] = (src[y*w + x] + src[y*w + x + 1]);
				dest[y*w + half + (x >> 1)] = ((11 * src[y*w + x] - 5 * src[y*w + x + 1]
					- 4 * src[y*w + x - 1] - 4 * src[y*w + x - 2]
					+ 1 * src[y*w + x - 3] + 1 * src[y*w + x - 4] + ROUNDING) >> 3);
			}
		}
	}
}


void InvertH26Wavelet(int *src, int *dest, int w, int h, int regw, int regh)
{
	int x, y;
	int half = regw;

	for (y = 0; y<regh * 2; y++)
	{
		for (x = 0; x<regw; x++)
		{
			if (x == 0)
			{
				dest[y*w + (x * 2)] = (((11 * src[y*w + x] - 4 * src[y*w + x + 1] + 1 * src[y*w + x + 2] + ROUNDING) >> 3) + (src[y*w + x + half])) >> 1;
				dest[y*w + (x * 2 + 1)] = (((5 * src[y*w + x] + 4 * src[y*w + x + 1] - 1 * src[y*w + x + 2] + ROUNDING) >> 3) - (src[y*w + x + half])) >> 1;
			}
			else if (x<regw - 1)
			{
				dest[y*w + (x * 2)] = (((src[y*w + x - 1] - src[y*w + x + 1] + ROUNDING) >> 3) + src[y*w + x] + (src[y*w + x + half])) >> 1;
				dest[y*w + (x * 2 + 1)] = (((-src[y*w + x - 1] + src[y*w + x + 1] + ROUNDING) >> 3) + src[y*w + x] - (src[y*w + x + half])) >> 1;
			}
			else
			{
				dest[y*w + (x * 2)] = (((5 * src[y*w + x] + 4 * src[y*w + x - 1] - 1 * src[y*w + x - 2] + ROUNDING) >> 3) + (src[y*w + x + half])) >> 1;
				dest[y*w + (x * 2 + 1)] = (((11 * src[y*w + x] - 4 * src[y*w + x - 1] + 1 * src[y*w + x - 2] + ROUNDING) >> 3) - (src[y*w + x + half])) >> 1;
			}
		}
	}
}



void Limit(int *dest, int w, int h, int max)
{
	int x, y;

	for (y = 0; y<h; y++)
	{
		for (x = 0; x<w; x++)
		{
			if (dest[y*w + x] < 0) dest[y*w + x] = 0;
			if (dest[y*w + x] > max) dest[y*w + x] = max;
		}
	}
}


void V26Wavelet(int *src, int *dest, int w, int h, int regw, int regh)
{
	int x, y;//, val;
	int half = w*(regh / 2);

	for (y = 0; y<regh; y += 2)
	{
		if (y == 0)
		{
			for (x = 0; x<regw; x++)
			{
				dest[(y >> 1)*w + x] = (src[y*w + x] + src[(y + 1)*w + x]);
				dest[(y >> 1)*w + x + half] = ((5 * src[y*w + x] - 11 * src[(y + 1)*w + x]
					+ 4 * src[(y + 2)*w + x] + 4 * src[(y + 3)*w + x]
					- 1 * src[(y + 4)*w + x] - 1 * src[(y + 5)*w + x] + ROUNDING) >> 3);
			}
		}
		else if (y<regh - 2)
		{
			for (x = 0; x<regw; x++)
			{
				dest[(y >> 1)*w + x] = (src[y*w + x] + src[(y + 1)*w + x]);
				dest[(y >> 1)*w + x + half] = ((-src[(y - 2)*w + x] - src[(y - 1)*w + x] + src[(y + 2)*w + x] + src[(y + 3)*w + x] + ROUNDING) >> 3) + src[y*w + x] - src[(y + 1)*w + x];
			}
		}
		else
		{
			for (x = 0; x<regw; x++)
			{
				dest[(y >> 1)*w + x] = (src[y*w + x] + src[(y + 1)*w + x]);
				dest[(y >> 1)*w + x + half] = ((11 * src[y*w + x] - 5 * src[(y + 1)*w + x]
					- 4 * src[(y - 1)*w + x] - 4 * src[(y - 2)*w + x]
					+ 1 * src[(y - 3)*w + x] + 1 * src[(y - 4)*w + x] + ROUNDING) >> 3);
			}
		}
	}
}


void InvertV26Wavelet(int *src, int *dest, int w, int h, int regw, int regh)
{
	int x, y;
	int half = w*regh;

	for (y = 0; y<regh; y++)
	{
		if (y == 0)
		{
			for (x = 0; x<regw * 2; x++)
			{
				dest[y * 2 * w + x] = (((11 * src[y*w + x] - 4 * src[(y + 1)*w + x] + 1 * src[(y + 2)*w + x] + ROUNDING) >> 3) + (src[y*w + x + half])) >> 1;
				dest[(y * 2 + 1)*w + x] = (((5 * src[y*w + x] + 4 * src[(y + 1)*w + x] - 1 * src[(y + 2)*w + x] + ROUNDING) >> 3) - (src[y*w + x + half])) >> 1;
			}
		}
		else if (y<regh - 1)
		{
			for (x = 0; x<regw * 2; x++)
			{
				dest[y * 2 * w + x] = (((src[(y - 1)*w + x] - src[(y + 1)*w + x] + ROUNDING) >> 3) + src[y*w + x] + (src[y*w + x + half])) >> 1;
				dest[(y * 2 + 1)*w + x] = (((-src[(y - 1)*w + x] + src[(y + 1)*w + x] + ROUNDING) >> 3) + src[y*w + x] - (src[y*w + x + half])) >> 1;
			}
		}
		else
		{
			for (x = 0; x<regw * 2; x++)
			{
				dest[y * 2 * w + x] = (((5 * src[y*w + x] + 4 * src[(y - 1)*w + x] - 1 * src[(y - 2)*w + x] + ROUNDING) >> 3) + (src[y*w + x + half])) >> 1;
				dest[(y * 2 + 1)*w + x] = (((11 * src[y*w + x] - 4 * src[(y - 1)*w + x] + 1 * src[(y - 2)*w + x] + ROUNDING) >> 3) - (src[y*w + x + half])) >> 1;
			}
		}
	}
}



void PrescaleLowPass(int *dest, int w, int h, int regw, int regh, int shift)
{
	int x, y;

	for (y = 0; y<regh; y++)
	{
		for (x = 0; x<regw; x++)
		{
			if (shift < 0)
			{
				dest[y*w + x] += (1 << (abs(shift) - 1)); //rounding
				dest[y*w + x] >>= abs(shift);
			}
			else
				dest[y*w + x] <<= shift;
		}
	}
}

void OutputSubbands(char *filename, int *src, int w, int h, int regw, int regh, int level)
{
	int i, x, y;
	char basename[256];
	char name[256];
	//int subband = level * 3;
	char band_name[3][3] = { "HL","LH","HH" };
	FILE *fp;

#ifdef _WINDOWS
	strcpy_s(basename, sizeof(name), filename);
	basename[strlen(basename) - 4] = 0;
#else
	strcpy(basename, filename);
	basename[strlen(basename) - 4] = 0;
#endif

	for(i=0; i<3; i++)
	{
#ifdef _WINDOWS
		sprintf_s(name, sizeof(name), "%s-%dx%d-band%s-L%d.raw", basename, regw, regh, band_name[i], level);
		fopen_s(&fp, name, "wb");
#else
		sprintf(name, "%s-%dx%d-band%s-L%d.raw", basename, regw, regh, band_name[i], level);
		fp = fopen(name, "wb");
#endif
		if (fp)
		{
			for (y = 0; y<regh; y++)
			{
				for (x = 0; x<regw; x++)
				{
					short val = src[y*w + x + regw];
					fwrite(&val, 1, 2, fp);
				}
			}
			fclose(fp);
		}
	}
}

void CopyBuff(int *src, int *dest, int w, int h)
{
	int  i;

	for (i = 0; i<w*h; i++)
	{
		*dest++ = *src++;
	}
}

void Stats(int *src, int w, int h, int regw, int regh)
{
	int x, y;
	int min = 0x7fffff, max = 0;

	for (y = 0; y<regh; y++)
	{
		for (x = 0; x<regw; x++)
		{
			if (src[y*w + x] < min)
				min = src[y*w + x];
			if (src[y*w + x] > max)
				max = src[y*w + x];
		}
	}
	printf("Low Pass (%d,%d) min = %d, max = %d\n", regw, regh, min, max);
}


double psnr(int *lptr, int *lptr2, int w, int h, int depth)
{
	double psnr;
	int x, y, val;
	double errorsquare = 0.0;

	for (y = 0; y < h; y++)
	{
		for (x = 0; x < w; x++)
		{
			val = (*lptr++ - *lptr2++);
			errorsquare += (double)(val * val);
		}
	}

	errorsquare /= (double)(w * h);
	psnr = 10.0 * log10(((double)(1 << depth)*(double)(1 << depth)) / errorsquare);

	return psnr;
}



void ExportPGM(char *filename, unsigned char *frameBuffer, int frameWidth, int frameHeight)
{
#ifdef _WINDOWS
	FILE *fp = NULL;
	fopen_s(&fp, filename, "wb");
#else
	FILE *fp = fopen(filename, "wb");
#endif
	if (fp)
	{
		fprintf(fp, "P5\n# %s\n", filename);
		fprintf(fp, "%d %d\n255\n", frameWidth, frameHeight);
		fwrite(frameBuffer, 1, frameWidth * frameHeight, fp);
		fclose(fp);
	}
}



void DiffBuffers(int *lptr, int *lptr2, unsigned char *cptr, int w, int h, int depth, int errorgain)
{
	int x, y, val;

	for (y = 0; y < h; y++)
	{
		for (x = 0; x < w; x++)
		{
			val = ((*lptr - *lptr2) >> (depth - 8))* errorgain + 128;
			if (val < 0) val = 0;
			if (val > 255) val = 255;
			*cptr++ = val;
			lptr++;
			lptr2++;
		}
	}
}

void ScaleBuffers(int *lptr, unsigned char *cptr, int w, int h, int scale)
{
	int x, y, val;

	for (y = 0; y < h; y++)
	{
		for (x = 0; x < w; x++)
		{
			val = (*lptr >> scale);
			if (val < 0) val = 0;
			if (val > 255) val = 255;
			*cptr++ = val;
			lptr++;
		}
	}
}


void ScaleThumbnail(int *lptr, unsigned char *cptr, int w, int h, int lowpass_w, int lowpass_h, int scaleLow)
{
	int x, y, val;

	for (y = 0; y < h; y++)
	{
		for (x = 0; x < w; x++)
		{
			if (y < lowpass_h && x < lowpass_w)
				val = (*lptr >> scaleLow);
			else
				val = 128 + (*lptr);

			if (val < 0) val = 0;
			if (val > 255) val = 255;
			*cptr++ = val;
			lptr++;
		}
	}
}

void GenerateDeepBufferFrom8bit(int *lptr, unsigned char *cptr, int source_w, int source_h, int w, int h, int depth)
{
	int y,i;

	for (y = 0; y < source_h; y++)
	{
		for (i = 0; i < source_w; i++)
		{
			*lptr++ = (((int)*cptr++) << (depth - 8)) + (rand() & ((1 << (depth - 8)) - 1)); // As we are using an 8-bit source inject noise into lower bits for deeper than 8-bit testing
		}
		for (; i < w; i++)
		{
			*lptr++ = 0;
		}
	}
	for (; y < h; y++)
	{
		for (i = 0; i < w; i++)
		{
			*lptr++ = 0;
		}
	}
}

void Init(quantizer_stats *s)
{
	int i;
	int mag, lastmag = 0;
	for (i = 0; i<256; i++)
	{
		mag = i + (i*i*i * 3) / (256 * 256);
		s->nonlinearquant[mag] = i;
	}

	for (i = 0; i<1025; i++)
	{
		if (s->nonlinearquant[i])
			lastmag = s->nonlinearquant[i];
		else
			s->nonlinearquant[i] = lastmag;
	}
	s->nonlinearquant[1024] = 256; //overflow

	for (i = 0; i<1024; i++)
	{
		s->invnonlinearquant[s->nonlinearquant[i]] = i;
	}
}
