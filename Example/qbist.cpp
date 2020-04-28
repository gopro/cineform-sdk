/*! @file qBist.cpp

*  @brief Render random-ish video frames for the encoder to compress via CineForm SDK
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

#include "stdafx.h"
#ifdef HAVE_OPENMP
#include <omp.h>
#define ANTIALIAS	1
#endif

static void
convertScanline(
	unsigned short *RGBAptr,
	unsigned char *outptr,
	int width,
	CFHD_PixelFormat pixelFormat,
	int     alpha)
{
	int R = 0, G = 0, B = 0;
	int x;
	unsigned char   *line8 = (unsigned char *)outptr;
	unsigned short  *line16 = (unsigned short *)outptr;
	unsigned int	*line32 = (unsigned int *)outptr;
	int shift = 0;
	int channels = 3, bitDepth = 8;

	channels = ChannelsInPixelFormat(pixelFormat);
	bitDepth = DepthInPixelFormat(pixelFormat);

	for (x = 0; x < width; x++)
	{
		unsigned short R, G, B;

		R = RGBAptr[0];
		G = RGBAptr[1];
		B = RGBAptr[2];
		RGBAptr += 4;
			
		switch(channels)
		{
		case 3:
			if (bitDepth == 8)
			{
				line8[0] = (unsigned char)(R >> 8);
				line8[1] = (unsigned char)(G >> 8);
				line8[2] = (unsigned char)(B >> 8);
				line8 += channels;
			}
			else if (bitDepth == 10)
			{
				unsigned int val;
				unsigned int r, g, b;

				r = R >> 6;
				g = G >> 6;
				b = B >> 6;

				if (pixelFormat == CFHD_PIXEL_FORMAT_R210)
				{
					val = (r << 20) | (g << 10) | (b);
					val = BYTESWAP32(val);
				}
				else if (pixelFormat == CFHD_PIXEL_FORMAT_DPX0)
				{
					val = (r << 22) | (g << 12) | (b << 2);
					val = BYTESWAP32(val);
				}
				else if (pixelFormat == CFHD_PIXEL_FORMAT_AB10)
				{
					val = (r << 20) | (g << 10) | (b);
				}
				else
				{
					val = r | (g << 10) | (b << 20);
				}

				*line32++ = val;
			}
			else
			{
				line16[0] = R;
				line16[1] = G;
				line16[2] = B;
				line16 += channels;
			}
			break;
		case 4:
			if (bitDepth == 8)
			{
				line8[0] = (unsigned char)(R >> 8);
				line8[1] = (unsigned char)(G >> 8);
				line8[2] = (unsigned char)(B >> 8);
				line8[3] = 255;

				if (alpha) // generate a fake alpha
				{
					int a = ((R+G+B)>>8) - 256;
					if (a < 0) a = 0;
					if (a > 255) a = 255;
					line8[3] = a;
				}
				line8 += channels;
			}
			else
			{
				//B64A - alpha first
				line16[0] = 65535;
				line16[1] = R;
				line16[2] = G;
				line16[3] = B;

				if (alpha) // generate a fake alpha
				{
					int a = ((R + G + B)) - 256;
					if (a < 0) a = 0;
					if (a > 65535) a = 255;
					line8[0] = a;
				}
				line16 += channels;
			}
			break;
		case 2: //YUV
			if (bitDepth == 8)
			{
				int r,g, b,y,u,v;
				r = (unsigned char)(R >> 8);
				g = (unsigned char)(G >> 8);
				b = (unsigned char)(B >> 8);

				y = (r * 183 + g * 614 + b * 62) / 1000 + 16;
				u = (-r * 101 - g * 338 + b * 439) / 1000 + 128;
				v = (r * 439 - g * 399 - b * 40) / 1000 + 128;

				if (y < 0) y = 0;
				if (y > 255) y = 255;
				if (u < 0) u = 0;
				if (u > 255) u = 255;
				if (v < 0) v = 0;
				if (v > 255) v = 255;
				line8[0] = y;
				if (x & 1)
					line8[1] = v;
				else
					line8[1] = u;

				line8 += channels;
			}
			else //YU64
			{
				int r, g, b, y, u, v;
				r = R;
				g = G;
				b = B;
				y = (r * 183 + g * 614 + b * 62) / 1000 + (16<<8);
				u = (-r * 101 - g * 338 + b * 439) / 1000 + 32768;
				v = (r * 439 - g * 399 - b * 40) / 1000 + 32768;

				if (y < 0) y = 0;
				if (y > 65535) y = 65535;
				if (u < 0) u = 0;
				if (u > 65535) u = 65535;
				if (v < 0) v = 0;
				if (v > 65535) v = 65535;

				line16[0] = y;
				if (x & 1)
					line16[1] = u;
				else
					line16[1] = v;
				line16 += channels;
			}
			break;
		}
	}
}



static void modify()
{
	unsigned short before[3 * 32 * 16],*ptr;
	unsigned short after[3 * 32 * 16];
	float diff;
	int delta;

	ptr = before;
	for (float y = 0.0; y < 0.999f; y += 1.0f / 16.0f)
	{
		for (float x = 0.0; x < 0.999f; x += 1.0f / 32.0f)
		{
			unsigned short r, g, b;
			QBist(x, y, &r, &g, &b);
			ptr[0] = r;
			ptr[1] = g;
			ptr[2] = b;
			ptr += 3;
		}
	}

	do
	{
		modifyQBistGenes();

		ptr = after;
		for (float y = 0.0; y < 0.999f; y += 1.0f / 16.0f)
		{
			for (float x = 0.0; x < 0.999f; x += 1.0f / 32.0f)
			{
				unsigned short r, g, b;
				QBist(x, y, &r, &g, &b);
				ptr[0] = r;
				ptr[1] = g;
				ptr[2] = b;
				ptr += 3;
			}
		}


		diff = PSNR(before, after, 32, 16,
			CFHD_PIXEL_FORMAT_RG48, 1);

		delta = 0; // check for solid color
		for (int row = 0; row < (32 * 16-1); row++)
		{
			delta += abs((int)after[row * 3] - (int)after[row * 3 + 3]);
			delta += abs((int)after[row * 3 + 1] - (int)after[row * 3 + 4]);
			delta += abs((int)after[row * 3 + 2] - (int)after[row * 3 + 5]);
		}

	} while (diff > 20.0f || delta < (32 * 16)*4000 || delta >(32 * 16) * 40000);
}



void RunQBist(int width, int height, int pitch, CFHD_PixelFormat pixelFormat, int alpha, unsigned char *ptr)
{
	int inverted = InvertedPixelFormat(pixelFormat);

#pragma omp parallel for
	for(int row=0; row<height; row++)
	{
		unsigned short *RGBAptr = (unsigned short *)ptr;
		float fy, fx;
		fy = (float)row / (float)(height);

		if(inverted)
			RGBAptr += (height - 1 - row) * width * 4;
		else
			RGBAptr += row * width * 4;

		for (int x = 0; x < width; x++)
		{
			unsigned short r, g, b;
			fx = (float)x / (float)(width);
			QBist(fx, fy, &r, &g, &b);

			RGBAptr[0] = r;
			RGBAptr[1] = g;
			RGBAptr[2] = b;
			RGBAptr[3] = 0xffff;

			RGBAptr += 4;
		}
	}

#if ANTIALIAS
#pragma omp parallel for
	for (int row = 1; row<height-1; row++)
	{
		unsigned short *RGBAptr = (unsigned short *)ptr;

		RGBAptr += row * width * 4 + 4;
		for (int x = 1; x < width-1; x++)
		{
			int xx,yy,chn, delta = 0;

			RGBAptr[0] &= ~1;
			for (yy = -1; yy <= 1; yy++)
			{
				for (xx = -1; xx <= 1; xx++)
				{
					for (chn = 0; chn <= 2; chn++)
					{
						delta = abs(RGBAptr[yy*width * 4 + xx * 4 + chn] - RGBAptr[chn]);
						if (delta > 8192)
						{
							RGBAptr[0] |= 1;
							goto next;
						}
					}
				}
			}
next:
			RGBAptr += 4;
		}
	}
	
	int convolve[3][3] =
	{
		{ 1, 2, 1, },
		{ 2, 4, 2, },
		{ 1, 2, 1, },
	};
#pragma omp parallel for
	for (int row = 1; row < height - 1; row++)
	{
		unsigned short *RGBAptr = (unsigned short *)ptr;
		float fy, fx;

		if (inverted)
			RGBAptr += (height - 1 - row) * width * 4 + 4;
		else
			RGBAptr += row * width * 4 + 4;

		for (int x = 1; x < width - 1; x++)
		{
			if(RGBAptr[0] & 1)
			{
				int R = 0, G = 0, B = 0;

				for (int yy = -1; yy <= 1; yy++)
				{
					fy = (float)(row * 2 + yy) / (float)(height * 2);
					for (int xx = -1; xx <= 1; xx++)
					{
						unsigned short r, g, b;
						fx = (float)(x * 2 + xx) / (float)(width * 2);

						QBist(fx, fy, &r, &g, &b);
						R += (int)r * convolve[yy + 1][xx + 1];
						G += (int)g * convolve[yy + 1][xx + 1];
						B += (int)b * convolve[yy + 1][xx + 1];
					}
				}

				R >>= 4;
				G >>= 4;
				B >>= 4;

				RGBAptr[0] = R;
				RGBAptr[1] = G;
				RGBAptr[2] = B;
			}

			RGBAptr += 4;
		}
	}
#endif

	for (int row = 0; row < height; row++)
	{
		unsigned short *RGBAptr = (unsigned short *)ptr;
		unsigned char *outptr = (unsigned char *)ptr;

		RGBAptr += width * 4 * row;
		outptr += pitch * row;

		convertScanline(RGBAptr, outptr, width, pixelFormat, alpha);
	}

	modify();
}
