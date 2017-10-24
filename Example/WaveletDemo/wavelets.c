/*! @file wavelets.c

*  @brief A simplistic wavelet compression modeling tool, similar to CineForm, for education purposes 
*   and some codec design and tuning. 
*
*  This models the 2-6 Wavelet, quanitization and reconstruction, massuring the image distortion, 
*  without any of the entropy encoding.  You can alter the waverlet order, quantization level and types. 
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


#define INTERLACED22		0		// Source is interlaced, use a different vertical filter on the first wavelet level

#define QUANTIZE_PRESCALE	1		// Precision will grow between wavelet layer, scale the Low pass between levels
#define QUANTIZE_HIGHPASS	1		//Compression is aided throught quantization

#define OUTPUT_DIFFERENCE	1		// Output a PGM (portable Grey Map) image for the differences due to quantization.
#define OUTPUT_WAVELET_TREE	1		// Output a PGM for the wavelet structure.
#define OUTPUT_DECODED		1		// Output a PGM for the decoded image.
#define SUBBANDS_STATS		1		// Show Subband stats
#define OUTPUTSUBBANDS		0		// Dump out the RAW qauntized Subband data.
#define ERRORGAIN			(10)	// The compression is good, this multiples the error to so can see the subtle differences

#define GENERATIONS			(1)		// The type of compression,setting losing little to nothing after the first generation.

#define LEVELS				(3)		// designed for 3, but you can experiment with 1 thru 7
#define BITDEPTH			(12)	// designed for 12, supports 8 thru 14

#if QUANTIZE_HIGHPASS  //Example qualitization tables.
	#if BITDEPTH==8 || BITDEPTH==9
	int quant_subband[/*LEVELS * 3*/] = { 8,8,12,   16,16,12, 48,48,24, 48,48,24, 48,48,24, 48,48,24, 48,48,24 };
	int prescale[/*LEVELS*/] = { 0, 0, 2, 2, 2, 2, 2 };
	#elif BITDEPTH==10
	int quant_subband[/*LEVELS * 3*/] = { 24,24,36, 12,12,6,  48,48,24, 48,48,24, 48,48,24, 48,48,24, 48,48,24 };
	int prescale[/*LEVELS*/] = { 0, 1, 2, 2, 2, 2, 2 };
	#elif BITDEPTH==11
	int quant_subband[/*LEVELS * 3*/] = { 24,24,36, 24,24,36,  48,48,24, 48,48,24, 48,48,24, 48,48,24, 48,48,24 };
	int prescale[/*LEVELS*/] = { 1, 2, 2, 2, 2, 2, 2 };
	#elif BITDEPTH==12
	int quant_subband[/*LEVELS * 3*/] = { 48,48,72, 48,48,24, 48,48,24, 48,48,24, 48,48,24, 48,48,24, 48,48,24 };
	int prescale[/*LEVELS*/] = { 1, 2, 2, 2, 2, 2, 2 };
	#elif BITDEPTH==13
	int quant_subband[/*LEVELS * 3*/] = { 48,48,72, 48,48,24, 48,48,24, 48,48,24, 48,48,24, 48,48,24, 48,48,24 };
	int prescale[/*LEVELS*/] = { 2, 2, 2, 2, 2, 2, 2 };
	#else 
	int quant_subband[/*LEVELS * 3*/] = { 48,48,72, 48,48,24, 48,48,24, 48,48,24, 48,48,24, 48,48,24, 48,48,24 };
	int prescale[/*LEVELS*/] = { 2, 2, 2, 2, 2, 2, 2 };
	#endif

#else
	#define LOSSLESS			(QUANTIZE_MULT==0 && QUANTIZE_NONLINEAR==0 && QUANTIZE_PRESCALE == 0)
#endif


#if LOSSLESS
 #define OFFSET 0
#else
 #define OFFSET	(64 >> (BITDEPTH - 8))
#endif



int main(int argc, char **argv)
{
	quantizer_stats s;

	memset((void *)&s, 0, sizeof(s));
	Init(&s);

	if(argc == 2)
	{
        unsigned char *buffer, *dest;
		char name[80], header[256] = {0};
		int len, w=0, h=0, source_w=0, source_h, i, count;
		FILE *fpin;

#ifdef _WINDOWS
		strcpy_s(name, sizeof(name), argv[1]);
		if (0 == fopen_s(&fpin, name, "rb"))
#else
		strcpy(name, argv[1]);
        fpin = fopen(name, "rb");
		if (fpin)
#endif
		{
			len = (int)fread(header, 1, 256, fpin);
			fclose(fpin);
		}

		if (header[0] == 'P') //PPM or PGM
		{
			if (header[1] != '5') //'5' = PGM //gray
			{
				printf("only PGMs currently supported\n");
				return 0;
			}
			count = 0;
			i = 0;
			while (i < len && count < 2)
			{
				if (header[i] == 10)
					count++;

				i++;
			}

#ifdef _WINDOWS
			sscanf_s(&header[i], "%d %d", &w, &h);
#else
			sscanf(&header[i], "%d %d", &w, &h);
#endif


			printf("source image size = %d,%d\n", w, h);

			// Resizing the image to suit the number of levels
			source_w = w;
			source_h = h;
			if (((w >> LEVELS) << LEVELS) != w)
			{
				w += 1 << LEVELS;
				w &= ~((1 << LEVELS) - 1);
			}
			if (((h >> LEVELS) << LEVELS) != h)
			{
				h += 1 << LEVELS;
				h &= ~((1 << LEVELS) - 1);
			}

			if(w && h && w < 16384 && h < 16384)
			{
                buffer = malloc(w*h+256);
				if(buffer)
				{
                    dest = malloc(w*h+256);
					if(dest)
					{
						int lowpass_w=0, lowpass_h=0;
						int *buffA = NULL;
						int *buffB = NULL;
						int *buffS = NULL;
						int *buffD = NULL;
						//int subband = 0;
						int header_length;
#ifdef _WINDOWS
						if (0 == fopen_s(&fpin, name, "rb"))
#else
                        fpin = fopen(name, "rb");
						if(fpin)
#endif
						{
							len = (int)fread(buffer, 1, (source_w*source_h) + 256, fpin);
							printf("source size = %d bytes\n", len);
							fclose(fpin);

							header_length = len - (source_w*source_h);

							memcpy(dest, buffer, header_length);

							buffA = malloc(w*h * 4);
							buffB = malloc(w*h * 4);
							buffS = malloc(w*h * 4);
							buffD = malloc(w*h * 4);

							if (buffA && buffB && buffS && buffD)
							{
								//int *lptr = buffA;
								unsigned char *cptr = &buffer[header_length];
								int regw = w, regh = h;
								int generations;

								// PGMs are only 8-bit, adding noise to simulate a deeper file.
								GenerateDeepBufferFrom8bit(buffA, cptr, source_w, source_h, w, h, BITDEPTH);

								//32bit copy of the deep source frame for PSNR testing
								CopyBuff(buffA, buffS, w, h); 

								for (generations = 0; generations < GENERATIONS; generations++)
								{
									int level;

									// ********************************
									// forward wavelet, i.e. encode
									// ********************************
									for (level = 0; level < LEVELS; level++)
									{
#if INTERLACED22
										if(level == 0)
											V22Wavelet(buffA, buffB, w, h, regw, regh);
										else
											V26Wavelet(buffA, buffB, w, h, regw, regh);
										H26Wavelet(buffB, buffA, w, h, regw, regh);
#else													
										H26Wavelet(buffA, buffB, w, h, regw, regh);
										V26Wavelet(buffB, buffA, w, h, regw, regh);
#endif
										regw /= 2; regh /= 2;

#if QUANTIZE_HIGHPASS
										QuantizeHighpass(buffA, w, h, regw, regh, quant_subband[level * 3 + 0], quant_subband[level * 3 + 1], quant_subband[level * 3 + 2], &s);
#endif

#if OUTPUTSUBBANDS
										OutputSubbands(argv[1], buffA, w, h, regw, regh, level);
#endif

#if QUANTIZE_PRESCALE
										if(level < LEVELS-1)
											PrescaleLowPass(buffA, w, h, regw, regh, -prescale[level]);
#endif
									}

									Stats(buffA, w, h, regw, regh);

									CopyBuff(buffA, buffD, w, h); //32bit copy for frame 1 for showing the wavelet
									lowpass_w = regw;
									lowpass_h = regh;

									// ********************************
									// inverse wavelet, i.e. decode
									// ********************************
									if (OFFSET != 0) // correct a DC offset due to rounding
										OffsetBuffer(buffA, w, h, regw, regh, OFFSET);

									for (level = LEVELS-1; level >= 0; level--)
									{
#if QUANTIZE_PRESCALE
										if (level < LEVELS-1)
											PrescaleLowPass(buffA, w, h, regw, regh, prescale[level]);
#endif

#if QUANTIZE_HIGHPASS
										InverseQuantizeHighpass(buffA, w, h, regw, regh, quant_subband[level * 3 + 0], quant_subband[level * 3 + 1], quant_subband[level * 3 + 2], &s);
#endif

#if INTERLACED22
										InvertH26Wavelet(buffA, buffB, w, h, regw, regh);
										if (level == 0)
											InvertV22Wavelet(buffB, buffA, w, h, regw, regh);
										else
											InvertV26Wavelet(buffB, buffA, w, h, regw, regh);
#else
										InvertV26Wavelet(buffA, buffB, w, h, regw, regh);
										InvertH26Wavelet(buffB, buffA, w, h, regw, regh);
#endif	
										regh *= 2; regw *= 2;
									}

									Limit(buffA, w, h, (1 << BITDEPTH) - 1);

									printf("\n");
								}


#if OUTPUT_WAVELET_TREE
								// Amplify the subbands so they can be viewed easier
								ScaleThumbnail(buffD, dest, w, h, lowpass_w, lowpass_h, ((BITDEPTH==8) ? 6 : 7));
#ifdef _WINDOWS
								strcpy_s(name, sizeof(name), argv[1]);
								name[strlen(name) - 4] = 0;
								strcat_s(name, sizeof(name), "-wavelet.pgm");
#else
								strcpy(name, argv[1]);
								name[strlen(name) - 4] = 0;
								strcat(name, "-wavelet.pgm");
#endif
								ExportPGM(name, dest, w, h);
#endif




#if OUTPUT_DECODED
								{
									ScaleBuffers(buffA, dest, w, h , (BITDEPTH - 8));
#ifdef _WINDOWS
									strcpy_s(name, sizeof(name), argv[1]); 	name[strlen(name) - 4] = 0;
									strcat_s(name, sizeof(name), "-decoded.pgm");
#else	
									strcpy(name, argv[1]);	name[strlen(name) - 4] = 0;
									strcat(name, "-decoded.pgm");
#endif	
									ExportPGM(name, dest, w, h);
								}
#endif

								printf("PSNR = %3.3f\n\n", psnr(buffA, buffS, w, h, BITDEPTH));								

#if OUTPUT_DIFFERENCE
								{
									DiffBuffers(buffA, buffS, dest, w, h, BITDEPTH, ERRORGAIN);
#ifdef _WINDOWS
									strcpy_s(name, sizeof(name), argv[1]); 	name[strlen(name) - 4] = 0;
									if (ERRORGAIN > 1)
									{
										char gaintxt[20];
										sprintf_s(gaintxt, sizeof(gaintxt), "-x%d", ERRORGAIN);
										strcat_s(name, sizeof(name), gaintxt);
									}
									strcat_s(name, sizeof(name), "-diff.pgm");
#else
									strcpy(name, argv[1]); 	name[strlen(name) - 4] = 0;
									if (ERRORGAIN > 1)
									{
										char gaintxt[20];
										sprintf(gaintxt, "-x%d", ERRORGAIN);
										strcat(name, gaintxt);
									}
									strcat(name, "-diff.pgm");
#endif
									ExportPGM(name, dest, w, h);
								}
#endif

								free(buffA);
								free(buffB);
								free(buffS);
								free(buffD);
							}
						}
						free(dest);
					}
					
					free(buffer);
				}
			}
		}
	}
	else
		printf("usage : %s [source_file.pgm]\n", argv[0]);	

}
