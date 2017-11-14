/*! @file utils.cpp

*  @brief Exerciser and example for using the CineForm SDK
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

#ifndef __APPLE__
#include <omp.h> 
#endif


unsigned int GetRand(unsigned int seed)
{
	static unsigned int w, z;
	if (seed) z = w = seed;
	z = 36969 * (z & 65535) + (z >> 16);
	w = 18000 * (w & 65535) + (w >> 16);
	return (z << 16) + w;
}

unsigned int myRand()
{
	return GetRand(0);
}

int ChannelsInPixelFormat(CFHD_PixelFormat pixelFormat)
{
	int channels = 3;

	if (pixelFormat == CFHD_PIXEL_FORMAT_BGRA || 
		pixelFormat == CFHD_PIXEL_FORMAT_BGRa || 
		pixelFormat == CFHD_PIXEL_FORMAT_B64A)
		channels = 4;

	if (pixelFormat == CFHD_PIXEL_FORMAT_YUY2 || 
		pixelFormat == CFHD_PIXEL_FORMAT_2VUY || 
		pixelFormat == CFHD_PIXEL_FORMAT_YU64)
		channels = 2;

	return channels;
}

int DepthInPixelFormat(CFHD_PixelFormat pixelFormat)
{
	int bitDepth = 8;

	if (pixelFormat == CFHD_PIXEL_FORMAT_B64A || 
		pixelFormat == CFHD_PIXEL_FORMAT_YU64 || 
		pixelFormat == CFHD_PIXEL_FORMAT_RG48)
		bitDepth = 16;

	if (pixelFormat == CFHD_PIXEL_FORMAT_R210 ||
		pixelFormat == CFHD_PIXEL_FORMAT_DPX0 ||
		pixelFormat == CFHD_PIXEL_FORMAT_AB10 ||
		pixelFormat == CFHD_PIXEL_FORMAT_AR10)
		bitDepth = 10;

	return bitDepth;
}

int FramePitch4PixelFormat(CFHD_PixelFormat pixelFormat, int frameWidth)
{
	int pitch;

	if (pixelFormat == CFHD_PIXEL_FORMAT_V210)	// 10-bit YUV 4:2:2
	{
		int width;

		// Force 16 byte alignment
		width = ((frameWidth + 47) / 48) * 48;

		// The v210 format uses 16 bytes to encode 6 pixels
		pitch = (width * 8) / 3;
	}
	else
	{
		int bitDepth = DepthInPixelFormat(pixelFormat);

		if (bitDepth == 10)
		{
			pitch = frameWidth * 4; // 30-bits within 32 -- all RGB formats 4:4:4
		}
		else
		{
			int channels = ChannelsInPixelFormat(pixelFormat);
			pitch = frameWidth * channels *(bitDepth / 8);
		}
	}

	return pitch;
}


int InvertedPixelFormat(CFHD_PixelFormat pixelFormat)
{
	int inverted = 0;

	if (pixelFormat == CFHD_PIXEL_FORMAT_BGRA || pixelFormat == CFHD_PIXEL_FORMAT_RG24)
		inverted = 1;

	return inverted;
}



void ExportPPM(char *filename, char *metadata, void *frameBuffer, int frameWidth, int frameHeight, int framePitch, CFHD_PixelFormat pixelFormat)
{
	int channels, bitDepth, inverted;
	void *newframe = NULL;

	channels = ChannelsInPixelFormat(pixelFormat);

	bitDepth = DepthInPixelFormat(pixelFormat);
	inverted = InvertedPixelFormat(pixelFormat);

	remove(filename);
#ifdef _WINDOWS
	FILE *fp = NULL;
	fopen_s(&fp, filename, "wb");
#else
	FILE *fp = fopen(filename, "wb");
#endif

	if (fp)
	{
		//unsigned char *ptr = (unsigned char *)frameBuffer;
		unsigned char line[65536], *dptr, *bptr = (unsigned char *)frameBuffer;

		fprintf(fp, "P6\n# %s\n", filename);
		if (metadata && strlen(metadata)> 0)
			fprintf(fp, "# %s\n", metadata);

		fprintf(fp, "%d %d\n255\n", frameWidth, frameHeight);

		if (bitDepth == 16)
		{
			unsigned short *sptr = (unsigned short *)frameBuffer;

			for (int j = 0; j < frameHeight; j++)
			{
				dptr = line;
				switch (channels)
				{
				case 4:
					for (int i = 0; i < frameWidth * channels; i += channels)
					{
						int alpha = sptr[i + 0] >> 8;
						int grey = 0x20 + 0x20 * (((i >> 8) + (j >> 6)) & 1);
						*dptr++ = ((sptr[i + 1] >> 8) * (alpha)+grey * (255 - alpha)) >> 8;
						*dptr++ = ((sptr[i + 2] >> 8) * (alpha)+grey * (255 - alpha)) >> 8;
						*dptr++ = ((sptr[i + 3] >> 8) * (alpha)+grey * (255 - alpha)) >> 8;
					}
					sptr += framePitch / 2;
					break;
				case 3:
					for (int i = 0; i < frameWidth * channels; i += channels)
					{
						*dptr++ = sptr[i + 0] >> 8;
						*dptr++ = sptr[i + 1] >> 8;
						*dptr++ = sptr[i + 2] >> 8;
					}
					sptr += framePitch / 2;
					break;
				case 2: //YU64

					{
						// Color conversion coefficients for 709 CG range
						int y_offset = 16 << 8;
						int c_offset = 32768;
						int ymult = 9535;		//1.164
						int r_vmult = 14688;	//1.793
						int g_vmult = 4383;		//0.534
						int g_umult = 1745;		//0.213
						int b_umult = 17326;	//2.115

						// Color conversion coefficients for 709 full range
						/*
						int y_offset = 0;
						uint32_t ymult = 8192;			// 1.0
						uint32_t r_vmult = 12616;		// 1.540
						uint32_t g_vmult = 3760;		// 0.459
						uint32_t g_umult = 1499;		// 0.183
						uint32_t b_umult = 14877;		// 1.816
						*/
						
						for (int i = 0; i < frameWidth * channels; i += channels * 2)
						{
							int Y1, U1, Y2, V1, R1, R2, G1, G2, B1, B2, r, g, b;


							Y1 = sptr[i + 0];
							V1 = sptr[i + 1];
							Y2 = sptr[i + 2];
							U1 = sptr[i + 3];

							Y1 -= y_offset;
							Y2 -= y_offset;
							U1 -= c_offset;
							V1 -= c_offset;

							R1 = ymult * Y1 + r_vmult * V1;
							R2 = ymult * Y2 + r_vmult * V1;
							B1 = ymult * Y1 + b_umult * U1;
							B2 = ymult * Y2 + b_umult * U1;
							G1 = ymult * Y1 - g_umult * U1 - g_vmult * V1;
							G2 = ymult * Y2 - g_umult * U1 - g_vmult * V1;

							r = R1 >> 21;
							g = G1 >> 21;
							b = B1 >> 21;
							if (r < 0) r = 0;
							if (r > 255) r = 255;
							if (g < 0) g = 0;
							if (g > 255) g = 255;
							if (b < 0) b = 0;
							if (b > 255) b = 255;
							*dptr++ = r;
							*dptr++ = g;
							*dptr++ = b;

							r = R2 >> 21;
							g = G2 >> 21;
							b = B2 >> 21;
							if (r < 0) r = 0;
							if (r > 255) r = 255;
							if (g < 0) g = 0;
							if (g > 255) g = 255;
							if (b < 0) b = 0;
							if (b > 255) b = 255;
							*dptr++ = r;
							*dptr++ = g;
							*dptr++ = b;
						}

						sptr += framePitch / 2;
					}
					break;
				}

				fwrite(line, 1, frameWidth * 3, fp);
			}
		}
		else if (bitDepth == 10)
		{
			unsigned int *lptr = (unsigned int *)frameBuffer;

			for (int j = 0; j < frameHeight; j++)
			{
				dptr = line;

				if (pixelFormat == CFHD_PIXEL_FORMAT_R210)
				{
					for (int i = 0; i < frameWidth; i++)
					{
						int swapped = BYTESWAP32(lptr[i]);

						*dptr++ = (swapped >> 22) & 0xff;
						*dptr++ = (swapped >> 12) & 0xff;
						*dptr++ = (swapped >> 2) & 0xff;
					}
				}
				else if (pixelFormat == CFHD_PIXEL_FORMAT_DPX0)
				{
					for (int i = 0; i < frameWidth; i++)
					{
						int swapped = BYTESWAP32(lptr[i]);

						*dptr++ = (swapped >> 24) & 0xff;
						*dptr++ = (swapped >> 14) & 0xff;
						*dptr++ = (swapped >> 4) & 0xff;
					}
				}
				else if (pixelFormat == CFHD_PIXEL_FORMAT_AB10)
				{
					for (int i = 0; i < frameWidth; i++)
					{
						*dptr++ = (lptr[i] >> 22) & 0xff;
						*dptr++ = (lptr[i] >> 12) & 0xff;
						*dptr++ = (lptr[i] >> 2) & 0xff;
					}
				}
				else if (pixelFormat == CFHD_PIXEL_FORMAT_AR10)
				{
					for (int i = 0; i < frameWidth; i++)
					{
						*dptr++ = (lptr[i] >> 2) & 0xff;
						*dptr++ = (lptr[i] >> 12) & 0xff;
						*dptr++ = (lptr[i] >> 22) & 0xff;
					}
				}
				else
				{
					for (int i = 0; i < frameWidth; i++)
					{
						*dptr++ = (lptr[i] >> 22) & 0xff;
						*dptr++ = (lptr[i] >> 12) & 0xff;
						*dptr++ = (lptr[i] >> 2) & 0xff;
					}
				}

				lptr += framePitch / 4;

				fwrite(line, 1, frameWidth * 3, fp);
			}
		}
		else
		{
			if (inverted)
			{
				bptr += (frameHeight - 1) * framePitch;
				framePitch = -framePitch;
			}

			switch (pixelFormat)
			{
			case CFHD_PIXEL_FORMAT_RG24: //inverted image pixel format 
				for (int j = 0; j < frameHeight; j++)
				{
					dptr = line;
					for (int i = 0; i < frameWidth * 3; i += 3)
					{
						*dptr++ = bptr[i + 0];
						*dptr++ = bptr[i + 1];
						*dptr++ = bptr[i + 2];
					}
					bptr += framePitch;
					fwrite(line, 1, frameWidth * 3, fp);
				}
				break;
			case CFHD_PIXEL_FORMAT_BGRA: //inverted image pixel format 
			case CFHD_PIXEL_FORMAT_BGRa:  // right size up version
				for (int j = 0; j < frameHeight; j++)
				{
					dptr = line;
					for (int i = 0; i < frameWidth * 4; i += 4)
					{
						*dptr++ = bptr[i + 0];
						*dptr++ = bptr[i + 1];
						*dptr++ = bptr[i + 2];
					}
					bptr += framePitch;
					fwrite(line, 1, frameWidth * 3, fp);
				}
				break;
			default:

				for (int j = 0; j < frameHeight; j++)
				{
					dptr = line;

					switch (channels)
					{
					case 4:
						for (int i = 0; i < frameWidth * channels; i += channels)
						{
							int alpha = bptr[i + 3];
							int grey = 0x20 + 0x20 * (((i >> 8) + (j >> 6)) & 1);
							*dptr++ = (bptr[i + 0] * (alpha)+grey * (255 - alpha)) >> 8;
							*dptr++ = (bptr[i + 1] * (alpha)+grey * (255 - alpha)) >> 8;
							*dptr++ = (bptr[i + 2] * (alpha)+grey * (255 - alpha)) >> 8;
						}
						bptr += framePitch;
						break;
					case 3:
						for (int i = 0; i < frameWidth * channels; i += channels)
						{
							*dptr++ = bptr[i + 0];
							*dptr++ = bptr[i + 1];
							*dptr++ = bptr[i + 2];
						}
						bptr += framePitch;
						break;
					case 2:
						{
							// Color conversion coefficients for 709 CG range
							int y_offset = 16;
							int c_offset = 128;
							int ymult = 9535;		//1.164
							int r_vmult = 14688;	//1.793
							int g_vmult = 4383;	//0.534
							int g_umult = 1745;	//0.213
							int b_umult = 17326;	//2.115


							for (int i = 0; i < frameWidth * channels; i += channels * 2)
							{
								int Y1, U1, Y2, V1, R1,R2, G1,G2, B1,B2 ,r,g,b;

								Y1 = (int)bptr[i + 0];
								U1 = (int)bptr[i + 1];
								Y2 = (int)bptr[i + 2];
								V1 = (int)bptr[i + 3];

								Y1 -= y_offset;
								Y2 -= y_offset;
								U1 -= c_offset;
								V1 -= c_offset;
								
								R1 = ymult * Y1 + r_vmult * V1;
								R2 = ymult * Y2 + r_vmult * V1;
								B1 = ymult * Y1 + b_umult * U1;
								B2 = ymult * Y2 + b_umult * U1;
								G1 = ymult * Y1 - g_umult * U1 - g_vmult * V1;
								G2 = ymult * Y2 - g_umult * U1 - g_vmult * V1;

								r = R1 >> 13;
								g = G1 >> 13;
								b = B1 >> 13;
								if (r < 0) r = 0;
								if (r > 255) r = 255;
								if (g < 0) g = 0;
								if (g > 255) g = 255;
								if (b < 0) b = 0;
								if (b > 255) b = 255;
								*dptr++ = r;
								*dptr++ = g;
								*dptr++ = b;

								r = R2 >> 13;
								g = G2 >> 13;
								b = B2 >> 13;
								if (r < 0) r = 0;
								if (r > 255) r = 255;
								if (g < 0) g = 0;
								if (g > 255) g = 255;
								if (b < 0) b = 0;
								if (b > 255) b = 255;
								*dptr++ = r;
								*dptr++ = g;
								*dptr++ = b;
							}
							bptr += framePitch;
						}
						break;
					}

					fwrite(line, 1, frameWidth * 3, fp);
				}
				break;
			}
		}

		fclose(fp);
	}

	if (newframe) free(newframe);
}






float PSNR(void *A, void *B, int width, int height, CFHD_PixelFormat pixelFormat, int scale)
{
	unsigned char *a = (unsigned char *)A;
	unsigned char *b = (unsigned char *)B; // decoded image, could be halfres
	unsigned short *a16 = (unsigned short *)A;
	unsigned short *b16 = (unsigned short *)B;
	unsigned int *a32 = (unsigned int *)A;
	unsigned int *b32 = (unsigned int *)B;
	double mse[4096];
	int shift = 0;
	int testchannels;
	int srcpitch;
	int decpitch;

	int channels, bitDepth;

	channels = ChannelsInPixelFormat(pixelFormat);
	bitDepth = DepthInPixelFormat(pixelFormat);

	testchannels = channels;
	srcpitch = width*channels;

	if (scale == 2) shift = 1;
	if (scale == 4) shift = 2;
	
	if (channels == 2) testchannels = 1; // test only luma

	for (int y = 0; y < height; y++)
		mse[y] = 0.0;
	
	decpitch = (width >> shift)*channels;

	if (bitDepth == 8)
	{
#pragma omp parallel for
		for (int y = 0; y < height >> shift; y++)
		{
			for (int x = 0; x < width >> shift; x++)
			{
				for (int c = 0; c < testchannels; c++)
				{
					int diff = 0;
					for (int yy = 0; yy < scale; yy++)
					{
						for (int xx = 0; xx < scale; xx++)
						{
							diff += a[((y << shift) + yy)*srcpitch + ((x << shift) + xx)*channels + c];
						}
					}

					diff -= b[y*decpitch + x*channels + c] * scale * scale;

					mse[y] += (double)(diff*diff) / (double)(scale*scale*scale*scale);
				}
			}
		}

		for (int y = 1; y < height >> shift; y++)
			mse[0] += mse[y];

		mse[0] /= (width >> shift)*(height >> shift)*testchannels;

		if (mse[0] == 0) return 999.0;
		return 10.0f * (float)log10(255.0 * 255.0 / mse[0]);
	}
	else if (bitDepth == 10)
	{
#pragma omp parallel for
		for (int y = 0; y < height >> shift; y++)
		{
			for (int x = 0; x < width >> shift; x++)
			{
				int r1=0, g1=0, b1=0;
				int r2, g2, b2;
				int samples = 1 << shift;

				if (pixelFormat == CFHD_PIXEL_FORMAT_R210) // BYTESWAP32((r << 20) | (g << 10) | (b));
				{
					for (int yy = 0; yy < samples; yy++)
					{
						for (int xx = 0; xx < samples; xx++)
						{
							r1 += (BYTESWAP32(a32[((y << shift) + yy)*width + (x << shift) + xx]) >> 20) & 0x3ff;
							g1 += (BYTESWAP32(a32[((y << shift) + yy)*width + (x << shift) + xx]) >> 10) & 0x3ff;
							b1 += (BYTESWAP32(a32[((y << shift) + yy)*width + (x << shift) + xx])) & 0x3ff;
						}
					}
					r2 = (BYTESWAP32(b32[y*(width >> shift) + x]) >> 20) & 0x3ff;
					g2 = (BYTESWAP32(b32[y*(width >> shift) + x]) >> 10) & 0x3ff;
					b2 = (BYTESWAP32(b32[y*(width >> shift) + x])) & 0x3ff;
					r2 *= scale * scale;
					g2 *= scale * scale;
					b2 *= scale * scale;

					mse[y] += (double)((r1 - r2)*(r1 - r2) + (g1 - g2)*(g1 - g2) + (b1 - b2)*(b1 - b2)) / (double)(3 * scale * scale * scale * scale);
				}
				else if (pixelFormat == CFHD_PIXEL_FORMAT_DPX0)  //BYTESWAP32((r << 22) | (g << 12) | (b << 2));
				{
					for (int yy = 0; yy < samples; yy++)
					{
						for (int xx = 0; xx < samples; xx++)
						{
							r1 += (BYTESWAP32(a32[((y << shift) + yy)*width + (x << shift) + xx]) >> 22) & 0x3ff;
							g1 += (BYTESWAP32(a32[((y << shift) + yy)*width + (x << shift) + xx]) >> 12) & 0x3ff;
							b1 += (BYTESWAP32(a32[((y << shift) + yy)*width + (x << shift) + xx]) >> 2) & 0x3ff;
						}
					}
					r2 = (BYTESWAP32(b32[y*(width >> shift) + x]) >> 22) & 0x3ff;
					g2 = (BYTESWAP32(b32[y*(width >> shift) + x]) >> 12) & 0x3ff;
					b2 = (BYTESWAP32(b32[y*(width >> shift) + x]) >> 2) & 0x3ff;
					r2 *= scale * scale;
					g2 *= scale * scale;
					b2 *= scale * scale;

					mse[y] += (double)((r1 - r2)*(r1 - r2) + (g1 - g2)*(g1 - g2) + (b1 - b2)*(b1 - b2)) / (double)(3 * scale * scale * scale * scale);
				}
				else if (pixelFormat == CFHD_PIXEL_FORMAT_AB10) // (r << 20) | (g << 10) | (b);
				{
					for (int yy = 0; yy < samples; yy++)
					{
						for (int xx = 0; xx < samples; xx++)
						{
							r1 += ((a32[((y << shift) + yy)*width + (x << shift) + xx]) >> 20) & 0x3ff;
							g1 += ((a32[((y << shift) + yy)*width + (x << shift) + xx]) >> 10) & 0x3ff;
							b1 += ((a32[((y << shift) + yy)*width + (x << shift) + xx]) >> 0) & 0x3ff;
						}
					}
					r2 = ((b32[y*(width >> shift) + x]) >> 20) & 0x3ff;
					g2 = ((b32[y*(width >> shift) + x]) >> 10) & 0x3ff;
					b2 = ((b32[y*(width >> shift) + x]) >> 0) & 0x3ff;
					r2 *= scale * scale;
					g2 *= scale * scale;
					b2 *= scale * scale;

					mse[y] += (double)((r1 - r2)*(r1 - r2) + (g1 - g2)*(g1 - g2) + (b1 - b2)*(b1 - b2)) / (double)(3 * scale * scale * scale * scale);
				}
				else  // r | (g << 10) | (b << 20);
				{
					for (int yy = 0; yy < samples; yy++)
					{
						for (int xx = 0; xx < samples; xx++)
						{
							r1 += ((a32[((y << shift) + yy)*width + (x << shift) + xx]) >> 0) & 0x3ff;
							g1 += ((a32[((y << shift) + yy)*width + (x << shift) + xx]) >> 10) & 0x3ff;
							b1 += ((a32[((y << shift) + yy)*width + (x << shift) + xx]) >> 20) & 0x3ff;
						}
					}
					r2 = ((b32[y*(width >> shift) + x]) >> 0) & 0x3ff;
					g2 = ((b32[y*(width >> shift) + x]) >> 10) & 0x3ff;
					b2 = ((b32[y*(width >> shift) + x]) >> 20) & 0x3ff;
					r2 *= scale * scale;
					g2 *= scale * scale;
					b2 *= scale * scale;

					mse[y] += (double)((r1 - r2)*(r1 - r2) + (g1 - g2)*(g1 - g2) + (b1 - b2)*(b1 - b2)) / (double)(3 * scale * scale * scale * scale);
				}
			}
		}

		for (int y = 1; y < height >> shift; y++)
			mse[0] += mse[y];

		mse[0] /= (width >> shift)*(height >> shift);

		if (mse[0] == 0) return 999.0;
		return 10.0f * (float)log10(1023.0*1023.0 / mse[0]);
	}
	else if(bitDepth == 16)
	{
#pragma omp parallel for
		for (int y = 0; y < height >> shift; y++)
			for (int x = 0; x < width >> shift; x++)
				for (int c = 0; c < testchannels; c++)
				{
					int diff = 0;
					for (int yy = 0; yy < scale; yy++)
					{
						for (int xx = 0; xx < scale; xx++)
						{
							diff += a16[((y << shift) + yy)*srcpitch + ((x << shift) + xx)*channels + c];
						}
					}

					diff -= b16[y*decpitch + x*channels + c] * scale * scale;

					mse[y] += (double)(diff*diff) / (double)(scale*scale*scale*scale);
				}

		for (int y = 1; y < height >> shift; y++)
			mse[0] += mse[y];

		mse[0] /= (width >> shift)*(height >> shift)*testchannels;

		if (mse[0] == 0) return 999.0;
		return 10.0f * (float)log10(65535.0*65535.0 / mse[0]);
	}

	return 0.f;
}
