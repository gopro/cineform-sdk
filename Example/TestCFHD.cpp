/*! @file TestCFHD.cpp

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
#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include <emmintrin.h>		// SSE2 intrinsics, _mm_malloc
#ifdef __APPLE__
#include <sys/time.h>
#else
#include <time.h>
#endif
#include "CFHDDecoder.h"
#include "CFHDEncoder.h"
#include "CFHDMetadata.h"

#ifdef _WIN32
#include <windows.h> // Performance counters
#endif

#include "mp4reader.h"

#define QBIST_SEED				50
#define ENABLE_3D				0		//2D or 3D-stereoscope encodign

#define QBIST_UNIQUE			1  //slower, but each frame get unique art to encode
#define OUTPUT_CFHD				0
#define DO_DECODE				1

#define OUTPUT_3D_TYPE			STEREO3D_TYPE_DEFAULT	
#if DO_DECODE
  #define DO_PSNR				1
  #if DO_PSNR  
    #define DECODE_FORMAT		pixelFormat			// Current encoding format, PSNR needs match pixel formats
    #define PPM_EXPORT_BELOW	30					// encoding 444 as 422 if a big PSNR hit
  #else
	#define DECODE_FORMAT		pixelFormat
    //#define DECODE_FORMAT   CFHD_PIXEL_FORMAT_RG48	// Specify a decode format, indepentent of encode type
    #define PPM_EXPORT_ALL	1
  #endif
#endif

#define EXPORT_DECODES_PPM	0

#ifdef _DEBUG
#define MAX_DEC_FRAMES		5
#define MAX_ENC_FRAMES		50
#define MAX_QUAL_FRAMES		1
#define	POOL_THREADS		4
#define	POOL_QUEUE_LENGTH	8

#define FRAME_WIDTH			1920
#define FRAME_HEIGHT		1080
#define BASENAME_IN			"FRMD"
#define BASENAME_OUT		"OUTD"
#else
#define MAX_DEC_FRAMES		50
#define MAX_ENC_FRAMES		500
#define MAX_QUAL_FRAMES		10
#define	POOL_THREADS		16
#define	POOL_QUEUE_LENGTH	24

#define FRAME_WIDTH			1920
#define FRAME_HEIGHT		1080
#define BASENAME_IN			"FRM"
#define BASENAME_OUT		"OUT"
#endif

unsigned int TestPixelFormat[] =
{ 
	//8-bit YUV 422 - 0
	CFHD_PIXEL_FORMAT_YUY2,			CFHD_ENCODED_FORMAT_YUV_422,		CFHD_ENCODING_QUALITY_FILMSCAN1,
	CFHD_PIXEL_FORMAT_2VUY,			CFHD_ENCODED_FORMAT_YUV_422,		CFHD_ENCODING_QUALITY_FILMSCAN1,
	
	//16-bit YUV 422		
	CFHD_PIXEL_FORMAT_YU64,			CFHD_ENCODED_FORMAT_YUV_422,		CFHD_ENCODING_QUALITY_FILMSCAN1,

	//8-bit RGB (inverted)
	CFHD_PIXEL_FORMAT_RG24,			CFHD_ENCODED_FORMAT_YUV_422,		CFHD_ENCODING_QUALITY_FILMSCAN1,  // Lower PSNR due to 4:2:2 subampling a 4:4:4 input.
	CFHD_PIXEL_FORMAT_RG24,			CFHD_ENCODED_FORMAT_RGB_444,		CFHD_ENCODING_QUALITY_FILMSCAN1,

	//8-bit RGBA - 4 (inverted)
	CFHD_PIXEL_FORMAT_BGRA,			CFHD_ENCODED_FORMAT_YUV_422,		CFHD_ENCODING_QUALITY_FILMSCAN1,  // Lower PSNR due to 4:2:2 subampling a 4:4:4 input.
	CFHD_PIXEL_FORMAT_BGRA,			CFHD_ENCODED_FORMAT_RGB_444,		CFHD_ENCODING_QUALITY_FILMSCAN1,
	CFHD_PIXEL_FORMAT_BGRA,			CFHD_ENCODED_FORMAT_RGBA_4444,		CFHD_ENCODING_QUALITY_FILMSCAN1,

	//8-bit RGBA - 4 (not-inverted)
	CFHD_PIXEL_FORMAT_BGRa,			CFHD_ENCODED_FORMAT_YUV_422,		CFHD_ENCODING_QUALITY_FILMSCAN1,  // Lower PSNR due to 4:2:2 subampling a 4:4:4 input.
	CFHD_PIXEL_FORMAT_BGRa,			CFHD_ENCODED_FORMAT_RGB_444,		CFHD_ENCODING_QUALITY_FILMSCAN1,
	CFHD_PIXEL_FORMAT_BGRa,			CFHD_ENCODED_FORMAT_RGBA_4444,		CFHD_ENCODING_QUALITY_FILMSCAN1,

	//10-bit RGB - 7 - TODO 10-bit RGB 
	CFHD_PIXEL_FORMAT_R210,			CFHD_ENCODED_FORMAT_RGB_444,		CFHD_ENCODING_QUALITY_FILMSCAN1,
	CFHD_PIXEL_FORMAT_DPX0,			CFHD_ENCODED_FORMAT_RGB_444,		CFHD_ENCODING_QUALITY_FILMSCAN1,
	CFHD_PIXEL_FORMAT_AB10,			CFHD_ENCODED_FORMAT_RGB_444,		CFHD_ENCODING_QUALITY_FILMSCAN1,
	CFHD_PIXEL_FORMAT_AR10,			CFHD_ENCODED_FORMAT_RGB_444,		CFHD_ENCODING_QUALITY_FILMSCAN1,

	//16-bit RGB - 15
	CFHD_PIXEL_FORMAT_RG48,			CFHD_ENCODED_FORMAT_YUV_422,		CFHD_ENCODING_QUALITY_FILMSCAN1,  // Lower PSNR due to 4:2:2 subampling a 4:4:4 input.
	CFHD_PIXEL_FORMAT_RG48,			CFHD_ENCODED_FORMAT_RGB_444,		CFHD_ENCODING_QUALITY_FILMSCAN1,

	//16-bit RGBA - 17 
	CFHD_PIXEL_FORMAT_B64A,			CFHD_ENCODED_FORMAT_YUV_422,		CFHD_ENCODING_QUALITY_FILMSCAN1,  // Lower PSNR due to 4:2:2 subampling a 4:4:4 input.
	CFHD_PIXEL_FORMAT_B64A,			CFHD_ENCODED_FORMAT_RGB_444,		CFHD_ENCODING_QUALITY_FILMSCAN1,
	CFHD_PIXEL_FORMAT_B64A,			CFHD_ENCODED_FORMAT_RGBA_4444,		CFHD_ENCODING_QUALITY_FILMSCAN1,

	//10-bit YUV 422 - 2 - TODO support V210 in TestCFHD
//	CFHD_PIXEL_FORMAT_V210,			CFHD_ENCODED_FORMAT_YUV_422,		CFHD_ENCODING_QUALITY_FILMSCAN1,

	//12-bit(Packed) Bayer/Raw - TODO BYR4/5 in TestCFHD
//	CFHD_PIXEL_FORMAT_BYR5,			CFHD_ENCODED_FORMAT_BAYER,			CFHD_ENCODING_QUALITY_FILMSCAN1,
//	//16-bit Bayer/Raw
//	CFHD_PIXEL_FORMAT_BYR4,			CFHD_ENCODED_FORMAT_BAYER,			CFHD_ENCODING_QUALITY_FILMSCAN1,

	0
};


CFHD_DecodedResolution TestResolution[] =
{
	CFHD_DECODED_RESOLUTION_FULL,
	CFHD_DECODED_RESOLUTION_HALF,
	CFHD_DECODED_RESOLUTION_QUARTER,
//	CFHD_DECODED_RESOLUTION_THUMBNAIL,
	CFHD_DECODED_RESOLUTION_UNKNOWN  // Stop at this
};

CFHD_PixelFormat TestDecodeOnlyPixelFormat[] =
{
	CFHD_PIXEL_FORMAT_RG24,
	CFHD_PIXEL_FORMAT_BGRA,
	CFHD_PIXEL_FORMAT_YUY2,
	CFHD_PIXEL_FORMAT_YU64,
	CFHD_PIXEL_FORMAT_RG48,
	CFHD_PIXEL_FORMAT_B64A,

	CFHD_PIXEL_FORMAT_R210,
	CFHD_PIXEL_FORMAT_DPX0,
	CFHD_PIXEL_FORMAT_AB10,
	CFHD_PIXEL_FORMAT_AR10,

	//10-bit YUV 422 - 2 - TODO support
	//	CFHD_PIXEL_FORMAT_V210,

	//12-bit(Packed) Bayer/Raw - TODO BY
	//	CFHD_PIXEL_FORMAT_BYR5,
	//	//16-bit Bayer/Raw
	//	CFHD_PIXEL_FORMAT_BYR4,

	CFHD_PIXEL_FORMAT_UNKNOWN  // Stop at this
};

#define PRINTF_PIXELFORMAT(k)			((k) >> 24) & 0xff, ((k) >> 16) & 0xff, ((k) >> 8) & 0xff, ((k) >> 0) & 0xff


double gettime(void)
{
#ifdef __APPLE__
	timeval ts;
	gettimeofday(&ts, NULL);
	return (double)ts.tv_sec + (double)ts.tv_usec / 1000000.0;
#elif _WIN32
	LARGE_INTEGER precision, ts;
	::QueryPerformanceCounter(&ts);
	::QueryPerformanceFrequency(&precision);
	return static_cast<double>(ts.QuadPart)/static_cast<double>(precision.QuadPart);
#else
	struct timespec ts;
	timespec_get(&ts, TIME_UTC);
	return (double)ts.tv_sec + (double)ts.tv_nsec / 1000000000.0;
#endif
}

void SDKVerion(CFHD_DecoderRef decoderRef, void *sampleBuffer, int sampleSize)
{
	int SDKverison = 0;
	int SAMPLEverison = 0;

	CFHD_GetSampleInfo(decoderRef,
		sampleBuffer,
		sampleSize,
		CFHD_SAMPLE_SDK_VERSION,
		(void *)&SDKverison,
		sizeof(SDKverison));
	CFHD_GetSampleInfo(decoderRef,
		sampleBuffer,
		sampleSize,
		CFHD_SAMPLE_ENCODE_VERSION,
		(void *)&SAMPLEverison,
		sizeof(SAMPLEverison));

	if (SDKverison)
		printf("SDK Version:  %d.%d.%d\n", SDKverison >> 16, (SDKverison >> 8) & 0xff, SDKverison & 0xff);
	if (SAMPLEverison)
		printf("Encoder Vers: %d.%d.%d\n", SAMPLEverison >> 16, (SAMPLEverison >> 8) & 0xff, SAMPLEverison & 0xff);
}


// Decode a single frame of a raw CineForm sample
CFHD_Error DecodeFrame(void **frameDecBuffer,
	CFHD_DecoderRef decoderRef,
	CFHD_MetadataRef metadataDecRef,
	void *sampleBuffer,
	int sampleSize,
	CFHD_EncodedFormat encodedFormat,
	CFHD_PixelFormat pixelFormat,
	CFHD_DecodedResolution resolution,
	char *outputname, // for PPM Export
	int *dec_us // Decode time in microseconds
)
{
	CFHD_Error error = CFHD_ERROR_OKAY;
	static CFHD_DecodedResolution lastres = CFHD_DECODED_RESOLUTION_UNKNOWN;
	static int actualWidth = 0;
	static int actualHeight = 0;
	static int actualPitch = 0;
	static uint32_t actualPixelSize = 0;
	static uint32_t actualOutputBufSize = 0;
	static uint32_t allocSize = 0;
	static CFHD_PixelFormat actualFormat = CFHD_PIXEL_FORMAT_UNKNOWN;
	static CFHD_PixelFormat lastpixelFormat = CFHD_PIXEL_FORMAT_UNKNOWN;
	static CFHD_EncodedFormat lastencFormat = CFHD_ENCODED_FORMAT_UNKNOWN;

	CFHD_VideoSelect videoselect = VIDEO_SELECT_BOTH_EYES;
	CFHD_Stereo3DType stereo3Dtype = OUTPUT_3D_TYPE;
	CFHD_StereoFlags stereoFlags = STEREO_FLAGS_DEFAULT;
	int videochannel = 1; //1 = 2D, 2 = 3D
	int scaleHeight = 1;  //1 = 2D, 2 = 3D 

	*dec_us = 0;

	if (*frameDecBuffer == NULL || lastres != resolution || lastencFormat != encodedFormat || lastpixelFormat != pixelFormat)
	{
		lastencFormat = encodedFormat;
		lastres = resolution;
		lastpixelFormat = pixelFormat;
		// Initialize the decoder
		error = CFHD_PrepareToDecode(decoderRef,
			0,
			0,
			pixelFormat,
			resolution,
			CFHD_DECODING_FLAGS_NONE,
			sampleBuffer,
			512,//sampleSize>>4,
			&actualWidth,
			&actualHeight,
			&actualFormat);
		if (error) return error;

		// Allocate a buffer for the decoded frame
		error = CFHD_GetPixelSize(actualFormat, &actualPixelSize); // Note: v210 will return 0 as it has not directly accessable pixel size.
		if (error) return error;

		error = CFHD_GetImagePitch(actualWidth, actualFormat, &actualPitch);
		if (error) return error;


		printf("\nDecode Res.:  %dx%d\n", actualWidth, actualHeight*scaleHeight);
				
		if (CFHD_ERROR_OKAY == CFHD_GetSampleInfo(decoderRef,
			sampleBuffer,
			sampleSize,
			CFHD_SAMPLE_INFO_CHANNELS,
			(void *)&videochannel,
			sizeof(videochannel)))
		{
			if (videochannel < 1) videochannel = 1;

			if (videochannel >= 2)
			{
				if (stereo3Dtype != STEREO3D_TYPE_DEFAULT)
					videoselect = VIDEO_SELECT_BOTH_EYES;
			}
			else
			{
				videoselect = VIDEO_SELECT_DEFAULT;
			}
			if (stereo3Dtype == STEREO3D_TYPE_DEFAULT && videoselect == VIDEO_SELECT_BOTH_EYES)
				scaleHeight = 2;
		}

		error = CFHD_GetImageSize(actualWidth, actualHeight, actualFormat,
			videoselect, stereo3Dtype, &actualOutputBufSize);
		if (error) return error;

#ifdef __APPLE__
		if (allocSize < actualOutputBufSize && *frameDecBuffer) free(*frameDecBuffer);
		if (allocSize < actualOutputBufSize || *frameDecBuffer == NULL)
		{
			*frameDecBuffer = malloc(actualOutputBufSize);;
			if (*frameDecBuffer == NULL)  
				return CFHD_ERROR_OUTOFMEMORY;
			allocSize = actualOutputBufSize;
		}
#else
		if(allocSize < actualOutputBufSize && *frameDecBuffer) _mm_free(*frameDecBuffer);
		if (allocSize < actualOutputBufSize || *frameDecBuffer == NULL)
		{
			*frameDecBuffer = _mm_malloc(actualOutputBufSize, 16);
			if (*frameDecBuffer == NULL)  
				return CFHD_ERROR_OUTOFMEMORY;
			allocSize = actualOutputBufSize;
		}
#endif

		CFHD_MetadataTrack metadataTrack;
		//metadataTrack = METADATATYPE_MODIFIED; // Use any active metadta changes created in GoPro Studio or CineForm FirstLight
		metadataTrack = METADATATYPE_ORIGINAL; // Don't use external active metadata, only internal corrections.
		error = CFHD_InitSampleMetadata(metadataDecRef,
			metadataTrack,
			sampleBuffer,
			sampleSize);

		// Override or set the active metadata processing used, this the mask that set feature image development features are enabled.
		unsigned int processflags = PROCESSING_ALL_ON;
		//unsigned int processflags = PROCESSING_ALL_OFF;		
		//unsigned int processflags = PROCESSING_ACTIVE_WHITEBALANCE | PROCESSING_ACTIVE_LOOK_FILE; // Lock and WB on only
		error = CFHD_SetActiveMetadata(decoderRef,
			metadataDecRef,
			TAG_PROCESS_PATH,
			METADATATYPE_UINT32,
			&processflags,
			sizeof(unsigned int));
		if (error) return error;
#ifdef _DEBUG
		unsigned int maxcpus = 1;
#else
		unsigned int maxcpus = 16;
#endif
		error = CFHD_SetActiveMetadata(decoderRef,
			metadataDecRef,
			TAG_CPU_MAX,
			METADATATYPE_UINT32,
			&maxcpus,
			sizeof(unsigned int));
		if (error) return error;


#if 1	
		// Initialize the 2D/3D output mode
		if (videochannel >= 2)
		{
			// Decode Left, Right or Both eyes.
			error = CFHD_SetActiveMetadata(decoderRef,
				metadataDecRef,
				TAG_CHANNELS_ACTIVE,
				METADATATYPE_UINT32,
				&videoselect,
				sizeof(unsigned int));

			// If both eyes use this stereo representation
			error = CFHD_SetActiveMetadata(decoderRef,
				metadataDecRef,
				TAG_CHANNELS_MIX,
				METADATATYPE_UINT32,
				&stereo3Dtype,
				sizeof(unsigned int));

			// Stereo flags, like, left/right eye swap -- rarely used
			error = CFHD_SetActiveMetadata(decoderRef,
				metadataDecRef,
				TAG_CHANNELS_MIX_VAL,
				METADATATYPE_UINT32,
				&stereoFlags,
				sizeof(unsigned int));

		}
#endif
	}

	if (resolution == CFHD_DECODED_RESOLUTION_THUMBNAIL)
	{
		size_t retWidth = 0;
		size_t retHeight = 0;
		size_t retSize = 0;

		error = CFHD_GetThumbnail(decoderRef,  // This only returns a DPX0 10-bit RGB thumbnail
			sampleBuffer,
			sampleSize,
			*frameDecBuffer,
			allocSize,
			CFHD_PIXEL_FORMAT_DPX0, // TODO: make this output other pixelformats
			&retWidth,
			&retHeight,
			&retSize);

#if EXPORT_DECODES_PPM
		if (error == 0 && outputname && outputname[0])
			ExportPPM(outputname, NULL, *frameDecBuffer, retWidth, retHeight, retWidth*4, CFHD_PIXEL_FORMAT_DPX0);
#endif
	}
	else
	{
		double uptime;
		double uptime2;
		uptime = gettime();
		error = CFHD_DecodeSample(decoderRef,
			sampleBuffer,
			sampleSize,
			*frameDecBuffer,
			actualPitch);
		uptime2 = gettime();
		if (error) 
			return error;

		*dec_us = (int)((uptime2 - uptime)*1000000.0);

#if EXPORT_DECODES_PPM
		if(outputname && outputname[0])
			ExportPPM(outputname, NULL, *frameDecBuffer, actualWidth, actualHeight*scaleHeight, actualPitch, pixelFormat);
#endif
	}

	if (error)
		return error;

	return CFHD_ERROR_OKAY;
}


// Decode a series of CineForm frames from an MOV or MP4 sequence
CFHD_Error DecodeMOVIE(char *filename, char *ext)
{
	CFHD_Error error = CFHD_ERROR_OKAY;
	CFHD_DecoderRef decoderRef = NULL;
	CFHD_MetadataRef metadataDecRef = NULL;
	int resmode = 0, decmode = 0, dec_us = 0;
	double dec_tot_us = 0;
	uint32_t *payload = NULL; //buffer to store samples from the MP4.
	void *frameDecBuffer = NULL;
	uint32_t AVI = 0;
	float length;
	void *handle;
	
#ifdef _WIN32
	if (0 == stricmp("AVI", ext))  AVI = 1;
#else
	if (0 == strcasecmp("AVI", ext))  AVI = 1;
#endif

	if(AVI)
		handle = OpenAVISource(filename, AVI_TRAK_TYPE, AVI_TRAK_SUBTYPE);
	else
		handle = OpenMP4Source(filename, MOV_TRAK_TYPE, MOV_TRAK_SUBTYPE);

	length = GetDuration(handle);

	if (length > 0.0)
	{
		int frame = 0;
		uint32_t numframes = GetNumberPayloads(handle);

		printf("found %.2fs of video (%d frames) within %s\n", length, numframes, filename);

		if (numframes > MAX_DEC_FRAMES)
			numframes = MAX_DEC_FRAMES;

		error = CFHD_OpenDecoder(&decoderRef, NULL);
		if (error) goto cleanup;

		error = CFHD_OpenMetadata(&metadataDecRef);
		if (error) goto cleanup;

		{
			frame = decmode = resmode = 0;
			do
			{
				char outputname[250] = "";
				char restxt[5] = "";
				CFHD_DecodedResolution decode_res = TestResolution[resmode];
				uint32_t payloadsize;
				CFHD_PixelFormat pixelFormat = TestDecodeOnlyPixelFormat[decmode];

				payloadsize = GetPayloadSize(handle, frame);
				payload = GetPayload(handle, payload, frame);

				if (payload == NULL)
				{
					error = CFHD_ERROR_OUTOFMEMORY;
					goto cleanup;
				}
#ifdef _WIN32 
				if (decode_res == 1) sprintf_s(restxt, sizeof(restxt), "FULL");
				else if (decode_res == 2) sprintf_s(restxt, sizeof(restxt), "HALF");
				else if (decode_res == 3) sprintf_s(restxt, sizeof(restxt), "QRTR");
				else sprintf_s(restxt, sizeof(restxt), "THUM");

				sprintf_s(outputname, sizeof(outputname), "%s-%s-%c%c%c%c-%04d.ppm", filename, restxt, (pixelFormat >> 24) & 0xff, (pixelFormat >> 16) & 0xff, (pixelFormat >> 8) & 0xff, (pixelFormat >> 0) & 0xff, frame);
#else

				if (decode_res == 1) sprintf(restxt, "FULL");
				else if (decode_res == 2) sprintf(restxt, "HALF");
				else if (decode_res == 3) sprintf(restxt, "QRTR");
				else sprintf(restxt, "THUM");

				sprintf(outputname, "%s-%s-%c%c%c%c-%04d.ppm", filename, restxt, (pixelFormat >> 24) & 0xff, (pixelFormat >> 16) & 0xff, (pixelFormat >> 8) & 0xff, (pixelFormat >> 0) & 0xff, frame);
#endif

				error = DecodeFrame(&frameDecBuffer, decoderRef, metadataDecRef, payload, (int)payloadsize, CFHD_ENCODED_FORMAT_UNKNOWN, pixelFormat, decode_res, outputname, &dec_us);
				if (error)
					goto cleanup;

				printf(".");
				dec_tot_us += (double)dec_us;

				if (frame < (int)numframes-1) 
					frame++;
				else
				{
					printf("\nAvg Decode time %.2fms for %c%c%c%c\n", (dec_tot_us / (double)frame)/1000.0, PRINTF_PIXELFORMAT(pixelFormat));
					dec_tot_us = 0;

					frame = 0;
					decmode++;
					if (TestDecodeOnlyPixelFormat[decmode] == CFHD_PIXEL_FORMAT_UNKNOWN && TestResolution[resmode + 1] != CFHD_DECODED_RESOLUTION_UNKNOWN)
					{
						resmode++;
						decmode = 0;
					}
				}




			} while (TestDecodeOnlyPixelFormat[decmode] != CFHD_PIXEL_FORMAT_UNKNOWN && TestResolution[resmode] != CFHD_DECODED_RESOLUTION_UNKNOWN);
		}

	}


cleanup:
	if (payload) FreePayload(payload); payload = NULL;
#ifdef __APPLE__
	if (frameDecBuffer) free(frameDecBuffer); frameDecBuffer = NULL;
#else
	if (frameDecBuffer) _mm_free(frameDecBuffer); frameDecBuffer = NULL;
#endif

	if (decoderRef) CFHD_CloseDecoder(decoderRef);
	if (metadataDecRef) CFHD_CloseMetadata(metadataDecRef);
	
	CloseSource(handle);

	return error;
}


CFHD_Error EncodeSpeedTest()
{
	int frmt = 0;
	CFHD_Error error = CFHD_ERROR_OKAY;
	CFHD_EncoderPoolRef encoderPoolRef = NULL;
	CFHD_MetadataRef metadataRef = NULL;
	CFHD_EncodingFlags encodingFlags = CFHD_ENCODING_FLAGS_NONE;
#if ENABLE_3D
	unsigned long videochannels = 2;  //1 - 2D, 2 - 3D
	unsigned long videochannel_gap = 0;
#else
	unsigned long videochannels = 1;  //1 - 2D, 2 - 3D
	unsigned long videochannel_gap = 0;
#endif

	bool firstVideoFrame = true;
	void *frameBuffer = NULL;
	int framePitch = 0;
	int frameWidth = 0;
	int frameHeight = 0;

	double tottime = 0.0;
	double tottime2;
	int queuedFrames = 0;
	int unique_frame = 0;
	
	do
	{
		int channels, bitDepth;
		int alpha = 0;
		CFHD_PixelFormat pixelFormat = (CFHD_PixelFormat)TestPixelFormat[frmt * 3 + 0];
		CFHD_EncodedFormat encodedFormat = (CFHD_EncodedFormat)TestPixelFormat[frmt * 3 + 1];
		CFHD_EncodingQuality quality = (CFHD_EncodingQuality)TestPixelFormat[frmt * 3 + 2];

		channels = ChannelsInPixelFormat(pixelFormat);
		bitDepth = DepthInPixelFormat(pixelFormat);
	
		if (encodedFormat == CFHD_ENCODED_FORMAT_RGBA_4444)
			alpha = 1;

		frameWidth = 0;
		frameHeight = 0;
		firstVideoFrame = true;

		error = CFHD_MetadataOpen(&metadataRef);
		if (error) return error;

		error = CFHD_CreateEncoderPool(&encoderPoolRef,
			POOL_THREADS,
			POOL_QUEUE_LENGTH,
			NULL);
		if (error) return error;

		// This is normally used to attach new metadata to the next frame to be encoded,
		// but here it simply attaches the allocator (if provided) to the metadataRef.
		CFHD_AttachEncoderPoolMetadata(encoderPoolRef, metadataRef);
		
		char stringpairA[] = "Director";
		char stringpairB[] = "John Doe";
		error = CFHD_MetadataAdd(metadataRef,
			TAG_NAME, METADATATYPE_STRING, strlen(stringpairA), (uint32_t *)stringpairA, false);
		error = CFHD_MetadataAdd(metadataRef,
			TAG_VALUE, METADATATYPE_STRING, strlen(stringpairB), (uint32_t *)stringpairB, false);
		error = CFHD_MetadataAdd(metadataRef,
			TAG_VIDEO_CHANNELS, METADATATYPE_UINT32, 4, (uint32_t *)&videochannels, false);
		if (videochannel_gap != 0)
			error = CFHD_MetadataAdd(metadataRef,
				TAG_VIDEO_CHANNEL_GAP, METADATATYPE_UINT32, 4, (uint32_t *)&videochannel_gap, false);

		// Prepare an unique image to encode
		GetRand(QBIST_SEED == 0 ? rand() : QBIST_SEED);
		initBaseTransform();

		int frameNumber;
		unique_frame = 0;
		for (frameNumber = 1; frameNumber <= MAX_ENC_FRAMES; )
		{
			if (error) break;

			// Is this the first video frame?
			if (frameNumber == 1)
			{
				// Get the frame dimensions
				frameWidth = FRAME_WIDTH;
				frameHeight = FRAME_HEIGHT;

				if (frameBuffer == NULL)
				{
#ifdef __APPLE__
					frameBuffer = malloc(frameWidth * frameHeight * 4 * 2);
#else
					frameBuffer = _mm_malloc(frameWidth * frameHeight * 4 * 2, 16); //Enough sapce for a 64-bit RGBA pixel
#endif
					if (frameBuffer == NULL)
					{
						error = CFHD_ERROR_OUTOFMEMORY;
						goto cleanup;
					}
				}

				// Initialize the encoder with the height of each channel
				int encodedHeight = (frameHeight - videochannel_gap) / videochannels;
				if (videochannels == 2)
					encodingFlags |= CFHD_ENCODING_FLAGS_LARGER_OUTPUT;


				// Prepare for encoding frames
				error = CFHD_PrepareEncoderPool(encoderPoolRef,
					frameWidth,
					encodedHeight,
					pixelFormat,
					encodedFormat,
					encodingFlags,
					quality);
				if (error) goto cleanup;

				// Attach the metadata to the encoder so it is added to the encoded samples
				CFHD_AttachEncoderPoolMetadata(encoderPoolRef, metadataRef);

				// Start the encoders in the pool of asynchronous encoders
				CFHD_StartEncoderPool(encoderPoolRef);

				// Generate a source image
				framePitch = frameWidth * channels *(bitDepth / 8);
				RunQBist(frameWidth, frameHeight, framePitch, pixelFormat, alpha, (unsigned char *)frameBuffer);

#if  PPM_EXPORT_ALL  // Output image prior to encoding -- the Before image.
				{
					char inputname[80] = "";
	#ifdef _WIN32 
					sprintf_s(inputname, sizeof(inputname), "%s-%04d.ppm", BASENAME_IN, frameNumber);
	#else
					sprintf(inputname, "%s-%04d.ppm", BASENAME_IN, frameNumber);
	#endif
					ExportPPM(inputname, NULL, frameBuffer, frameWidth, frameHeight, framePitch, pixelFormat);
				}
#endif
				tottime = gettime();

			}


			if (queuedFrames < POOL_QUEUE_LENGTH)
			{
				static int base = 24;
				static int frms = 1 * 60 * 60 * base; // demo, start at 01:00:00:00
				int f, s, m, h;
				char tc[12];

				// attached changing metadata like timecode, unique_frame number and 
				// any other live camera feed data.
				f = frms % base;
				s = ((frms) / base) % 60;
				m = ((frms) / (base * 60)) % 60;
				h = ((frms) / (base * 60 * 60)) % 24;

				frms += 1; // or whatever the frame number will be for a threaded implementation

						   // Timecode should be set for every frame encoded.  
						   // If your metadata is unchanging (other than timecode), and you aren't threading 
						   // the encoder, timecode and timecode_base only need to be set for the first frame 
						   // and it will auto-increment.   
				sprintf(tc, "%02d:%02d:%02d:%02d", h, m, s, f);
				error = CFHD_MetadataAdd(metadataRef,
					TAG_TIMECODE, METADATATYPE_STRING, 11, (uint32_t *)tc, false);


				// Like timecode UNIQUE_FRAMENUM should be set for every frame encoded.  
				// It ranges form 0 up to the number of frames in the encoded sequence. 
				// If you aren't threading the encoder, unique frame will auto-increment.   
				error = CFHD_MetadataAdd(metadataRef,
					TAG_UNIQUE_FRAMENUM, METADATATYPE_UINT32, 4, (uint32_t *)&unique_frame, false);
				unique_frame++;

				error = CFHD_EncodeAsyncSample(encoderPoolRef, unique_frame, frameBuffer, framePitch, metadataRef);
				if (error) goto cleanup;

				queuedFrames++;
			}


			if (queuedFrames)
			{
				uint32_t frame_number;
				CFHD_SampleBufferRef sampleBufferRef = NULL;

				if (unique_frame == 1)
				{
					printf("Resolution:   %dx%d\n", FRAME_WIDTH, FRAME_HEIGHT);
					printf("Pixel format: %c%c%c%c\n", (pixelFormat >> 24) & 0xff, (pixelFormat >> 16) & 0xff, (pixelFormat >> 8) & 0xff, (pixelFormat >> 0) & 0xff);
					printf("Encode:       %d\n", encodedFormat == CFHD_ENCODED_FORMAT_YUV_422 ? 422 : encodedFormat == CFHD_ENCODED_FORMAT_RGB_444 ? 444 : encodedFormat == CFHD_ENCODED_FORMAT_RGBA_4444 ? 4444 : 0);
				}

				if (CFHD_ERROR_OKAY == CFHD_TestForSample(encoderPoolRef, &frame_number, &sampleBufferRef))
				{
					// Get the sample data from the sample buffer
					unsigned char *sampleBuffer;
					size_t sampleSize;
					error = CFHD_GetEncodedSample(sampleBufferRef, (void **)&sampleBuffer, &sampleSize);
					if (error) goto cleanup;
					printf(".");
					if (((frame_number - 1) & 63) == 63) printf("\n");

					frameNumber++;
					queuedFrames--;

					// Release the sample buffer
					CFHD_ReleaseSampleBuffer(encoderPoolRef, sampleBufferRef);
				}
			}

			if (frameNumber == MAX_ENC_FRAMES)
			{
				while (queuedFrames)
				{
					uint32_t frame_number;
					CFHD_SampleBufferRef sampleBufferRef = NULL;

					if (CFHD_ERROR_OKAY == CFHD_WaitForSample(encoderPoolRef, &frame_number, &sampleBufferRef))
					{	// Get the sample data from the sample buffer
						unsigned char *sampleBuffer;
						size_t sampleSize;
						error = CFHD_GetEncodedSample(sampleBufferRef, (void **)&sampleBuffer, &sampleSize);
						if (error) goto cleanup;

						printf(".");
						if (((frame_number - 1) & 63) == 63) printf("\n");
						frameNumber++;
						queuedFrames--;

						// Release the sample buffer
						CFHD_ReleaseSampleBuffer(encoderPoolRef, sampleBufferRef);
					}
				}
			}
		}

		tottime2 = gettime();
		tottime = (tottime2 - tottime);
		tottime /= (double)MAX_ENC_FRAMES;
		printf("\n%d frames %1.2fms per frame (%1.1ffps)\n", MAX_ENC_FRAMES, tottime*1000.0, 1.0 / tottime);

		// Free the encoder
		CFHD_ReleaseEncoderPool(encoderPoolRef);
		encoderPoolRef = NULL;

		frmt++;
		printf("\n");

	} while (TestPixelFormat[frmt * 3]);

cleanup:
	if (error)
	{
#ifdef __APPLE__
		if (frameBuffer) free(frameBuffer), frameBuffer = NULL;
#else
		if (frameBuffer) _mm_free(frameBuffer), frameBuffer = NULL;
#endif
	}

	if (encoderPoolRef) CFHD_CloseEncoder(encoderPoolRef);

	return error;
}



CFHD_Error EncodeDecodeQualityTest()
{
	int frmt = 0;
	CFHD_Error error = CFHD_ERROR_OKAY;
	CFHD_EncoderRef encoderRef = NULL;
	CFHD_DecoderRef decoderRef = NULL;
	CFHD_MetadataRef metadataRef = NULL;
	CFHD_MetadataRef metadataDecRef = NULL;
	CFHD_EncodingFlags encodingFlags = CFHD_ENCODING_FLAGS_NONE;
#if ENABLE_3D
	unsigned long videochannels = 2;  //1 - 2D, 2 - 3D
	unsigned long videochannel_gap = 0;
#else
	unsigned long videochannels = 1;  //1 - 2D, 2 - 3D
	unsigned long videochannel_gap = 0;
#endif

	bool firstVideoFrame = true;
	void *frameBuffer = NULL;
	void *frameDecBuffer = NULL;
	int framePitch = 0;
	int frameWidth = 0;
	int frameHeight = 0;

	double uptime;
	double uptime2;
	int dec_us, enc_us;
	int resmode = 0, decmode = 0;

	do
	{
		CFHD_DecodedResolution decode_res = TestResolution[resmode];
		int channels, bitDepth, inverted;
		int alpha = 0;
		CFHD_PixelFormat pixelFormat = (CFHD_PixelFormat)TestPixelFormat[frmt * 3 + 0];
		CFHD_EncodedFormat encodedFormat = (CFHD_EncodedFormat)TestPixelFormat[frmt * 3 + 1];
		CFHD_EncodingQuality quality = (CFHD_EncodingQuality)TestPixelFormat[frmt * 3 + 2];
		char restxt[5] = "";
		char enctxt[5] = "";
		char inputname[80] = "";
		char outputname[80] = "";

#ifdef _WIN32
		if (decode_res == 1) sprintf_s(restxt, sizeof(restxt), "FULL");
		else if (decode_res == 2) sprintf_s(restxt, sizeof(restxt), "HALF");
		else if (decode_res == 3) sprintf_s(restxt, sizeof(restxt), "QRTR");

		if (encodedFormat == CFHD_ENCODED_FORMAT_YUV_422) 	sprintf_s(enctxt, sizeof(restxt), "422");
		else if (encodedFormat == CFHD_ENCODED_FORMAT_RGB_444)	sprintf_s(enctxt, sizeof(restxt), "444");
		else if (encodedFormat == CFHD_ENCODED_FORMAT_RGBA_4444)	sprintf_s(enctxt, sizeof(restxt), "4444");
#else
		if (decode_res == 1) sprintf(restxt, "FULL");
		else if (decode_res == 2) sprintf(restxt, "HALF");
		else if (decode_res == 3) sprintf(restxt, "QRTR");

		if (encodedFormat == CFHD_ENCODED_FORMAT_YUV_422) 	sprintf(enctxt, "422");
		else if (encodedFormat == CFHD_ENCODED_FORMAT_RGB_444)	sprintf(enctxt, "444");
		else if (encodedFormat == CFHD_ENCODED_FORMAT_RGBA_4444)	sprintf(enctxt, "4444");
#endif

		channels = ChannelsInPixelFormat(pixelFormat);
		bitDepth = DepthInPixelFormat(pixelFormat);
		inverted = InvertedPixelFormat(pixelFormat);

		if (encodedFormat == CFHD_ENCODED_FORMAT_RGBA_4444)
			alpha = 1;

		frameWidth = 0;
		frameHeight = 0;
		firstVideoFrame = true;

		error = CFHD_MetadataOpen(&metadataRef);
		if (error) return error;

		error = CFHD_OpenEncoder(&encoderRef, NULL);
		if (error) return error;

		error = CFHD_OpenDecoder(&decoderRef, NULL);
		if (error)	return error;

		error = CFHD_OpenMetadata(&metadataDecRef);
		if (error) return error;


		char stringpairA[] = "Director";
		char stringpairB[] = "John Doe";
		error = CFHD_MetadataAdd(metadataRef,
			TAG_NAME, METADATATYPE_STRING, strlen(stringpairA), (uint32_t *)stringpairA, false);
		error = CFHD_MetadataAdd(metadataRef,
			TAG_VALUE, METADATATYPE_STRING, strlen(stringpairB), (uint32_t *)stringpairB, false);
		error = CFHD_MetadataAdd(metadataRef,
			TAG_VIDEO_CHANNELS, METADATATYPE_UINT32, 4, (uint32_t *)&videochannels, false);
		if (videochannel_gap != 0)
			error = CFHD_MetadataAdd(metadataRef,
				TAG_VIDEO_CHANNEL_GAP, METADATATYPE_UINT32, 4, (uint32_t *)&videochannel_gap, false);


		// Prepare an unique image to encode
		GetRand(QBIST_SEED == 0 ? rand() : QBIST_SEED);
		initBaseTransform();

		int frameNumber;
		for (frameNumber = 1; frameNumber <= MAX_QUAL_FRAMES; )
		{
#ifdef _WIN32 
			//sprintf_s(inputname, sizeof(inputname), "%s-%04d.ppm", BASENAME_IN, frameNumber);
			sprintf_s(inputname, sizeof(inputname), "%s-%c%c%c%c%c-%s-%s-%04d.ppm", BASENAME_IN, (pixelFormat >> 24) & 0xff, (pixelFormat >> 16) & 0xff, (pixelFormat >> 8) & 0xff, (pixelFormat >> 0) & 0xff, (inverted ? 'i' : '-'), restxt, enctxt, frameNumber);
			sprintf_s(outputname, sizeof(outputname), "%s-%c%c%c%c%c-%s-%s-%04d.ppm", BASENAME_OUT, (pixelFormat >> 24) & 0xff, (pixelFormat >> 16) & 0xff, (pixelFormat >> 8) & 0xff, (pixelFormat >> 0) & 0xff, (inverted ? 'i' : '-'), restxt, enctxt, frameNumber);
#else
			sprintf(inputname, "%s-%04d.ppm", BASENAME_IN, frameNumber);
			sprintf(outputname, "%s-%c%c%c%c-%s-%s-%04d.ppm", BASENAME_OUT, (pixelFormat >> 24) & 0xff, (pixelFormat >> 16) & 0xff, (pixelFormat >> 8) & 0xff, (pixelFormat >> 0) & 0xff, restxt, enctxt, frameNumber);

#endif
			if (error) break;

			// Is this the first video frame?
			if (frameNumber == 1)
			{
				// Get the frame dimensions
				frameWidth = FRAME_WIDTH;
				frameHeight = FRAME_HEIGHT;

				if (frameBuffer == NULL)
				{
#ifdef __APPLE__
					frameBuffer = malloc(frameWidth * frameHeight * 4 * 2);
#else
					frameBuffer = _mm_malloc(frameWidth * frameHeight * 4 * 2, 16); //Enough sapce for a 64-bit RGBA pixel
#endif
					if (frameBuffer == NULL)
					{
						error = CFHD_ERROR_OUTOFMEMORY;
						goto cleanup;
					}
				}

				// Initialize the encoder with the height of each channel
				int encodedHeight = (frameHeight - videochannel_gap) / videochannels;
				if (videochannels == 2)
					encodingFlags |= CFHD_ENCODING_FLAGS_LARGER_OUTPUT;


				// Prepare for encoding frames
				error = CFHD_PrepareToEncode(encoderRef,
					frameWidth,
					encodedHeight,
					pixelFormat,
					encodedFormat,
					encodingFlags,
					quality);
				if (error) goto cleanup;

				// Attach the metadata to the encoder so it is added to the encoded samples
				CFHD_MetadataAttach(encoderRef, metadataRef);

				// Generate a source image
				framePitch = FramePitch4PixelFormat(pixelFormat, frameWidth);
				RunQBist(frameWidth, frameHeight, framePitch, pixelFormat, alpha, (unsigned char *)frameBuffer);

#if  PPM_EXPORT_ALL  // Output image prior to encoding -- the Before image.
				ExportPPM(inputname, NULL, frameBuffer, frameWidth, frameHeight, framePitch, pixelFormat);
#endif

			}

#if QBIST_UNIQUE
			//Generate a new source image
			if (frameNumber > 1)
				RunQBist(frameWidth, frameHeight, framePitch, pixelFormat, alpha, (unsigned char *)frameBuffer);
#endif

			uptime = gettime();
			// Encode the video frame
			error = CFHD_EncodeSample(encoderRef, frameBuffer, framePitch);
			if (error) goto cleanup;

			void *sampleBuffer;
			size_t sampleSize;
			int scale = 1 << ((int)decode_res - 1);

			error = CFHD_GetSampleData(encoderRef, (void **)&sampleBuffer, &sampleSize);
			if (error) goto cleanup;

			uptime2 = gettime();
			enc_us = (unsigned int)((uptime2 - uptime)*1000000.0);

#if OUTPUT_CFHD
			{
				char name[20];
				sprintf(name, "%s%04d.cfhd", BASENAME_OUT, frameNumber);
				FILE *fp = fopen(name, "wb");
				if (fp) fwrite(sampleBuffer, 1, sampleSize, fp);
				fclose(fp);
			}
#endif
			// Print the SDK version flagged in the bit-stream
			if (frameNumber == 1)
			{
				static int once = 0;
				if(once == 0)
					SDKVerion(decoderRef, sampleBuffer, (int)sampleSize), once++;
				printf("Resolution:   %dx%d\n", FRAME_WIDTH, FRAME_HEIGHT);
#ifdef DECODE_FORMAT
				if (pixelFormat != DECODE_FORMAT)
					printf("Pixel format: %c%c%c%c <--> %c%c%c%c\n", (pixelFormat >> 24) & 0xff, (pixelFormat >> 16) & 0xff, (pixelFormat >> 8) & 0xff, (pixelFormat >> 0) & 0xff, (DECODE_FORMAT >> 24) & 0xff, (DECODE_FORMAT >> 16) & 0xff, (DECODE_FORMAT >> 8) & 0xff, (DECODE_FORMAT >> 0) & 0xff);
				else
#endif
					printf("Pixel format: %c%c%c%c\n", (pixelFormat >> 24) & 0xff, (pixelFormat >> 16) & 0xff, (pixelFormat >> 8) & 0xff, (pixelFormat >> 0) & 0xff);
				printf("Encode:       %d\n", encodedFormat == CFHD_ENCODED_FORMAT_YUV_422 ? 422 : encodedFormat == CFHD_ENCODED_FORMAT_RGB_444 ? 444 : encodedFormat == CFHD_ENCODED_FORMAT_RGBA_4444 ? 4444 : 0);
				printf("Decode:       %s\n", decode_res == 1 ? "Full res" : decode_res == 2 ? "Half res" : decode_res == 3 ? "Quarter res" : "none");
			}

#if DO_DECODE
			error = DecodeFrame(&frameDecBuffer, decoderRef, metadataDecRef, sampleBuffer, (int)sampleSize, encodedFormat, DECODE_FORMAT, decode_res, NULL, &dec_us);
			if (error) goto cleanup;

#if DO_PSNR // export frames that have more that xdB of PSNR error.
			if (1)
			{
				float psnr;
				int sourcesize = framePitch*frameHeight;

				psnr = PSNR(frameBuffer, frameDecBuffer, frameWidth, frameHeight, pixelFormat, scale);
				printf("%d: source %d compressed to %d in %1.1fms - %1.1fms (%1.1f:1 PSNR %1.1fdB)\n", frameNumber, sourcesize, (int)sampleSize, (float)enc_us / 1000.0f, (float)dec_us / 1000.0f, (float)sourcesize / (float)sampleSize, psnr);

				if (psnr < PPM_EXPORT_BELOW)
				{
					char metadata[64];
#ifdef _WIN32 
					sprintf_s(metadata, sizeof(metadata), "PSNR = %f", psnr);
#else
					sprintf(metadata, "PSNR = %f", psnr);
#endif
					//ExportPPM(inputname, metadata, frameBuffer, frameWidth, frameHeight, framePitch, pixelFormat);
					ExportPPM(outputname, metadata, frameDecBuffer, frameWidth / scale, frameHeight / scale, framePitch / scale, pixelFormat);
				}
			}
#else
			printf("%d: source %d compressed to %d in %1.1fms - %1.1fms (%1.1f:1)\n", frameNumber, framePitch*frameHeight, sampleSize, (float)enc_us / 1000.0f, (float)dec_us / 1000.0f, (float)(framePitch*frameHeight) / (float)sampleSize);
#endif

#endif // DO_DECODE

			frameNumber++;
		}

		// Free the encoder
		CFHD_CloseEncoder(encoderRef);
		encoderRef = NULL;

		frmt++;
		if (TestPixelFormat[frmt * 3] == 0 && TestResolution[resmode + 1] != CFHD_DECODED_RESOLUTION_UNKNOWN)
		{
			resmode++;
			frmt = 0;
		}
		printf("\n");

	} while (TestPixelFormat[frmt * 3] && TestResolution[resmode] != CFHD_DECODED_RESOLUTION_UNKNOWN);

cleanup:
	if (error)
	{

#ifdef __APPLE__
		if (frameBuffer) free(frameBuffer), frameBuffer = NULL;
		if (frameDecBuffer) free(frameDecBuffer), frameDecBuffer = NULL;
#else
		if (frameBuffer) _mm_free(frameBuffer), frameBuffer = NULL;
		if (frameDecBuffer) _mm_free(frameDecBuffer), frameDecBuffer = NULL;
#endif
	}


	if (decoderRef) CFHD_CloseDecoder(decoderRef);
	if (encoderRef) CFHD_CloseEncoder(encoderRef);

	return error;
}




int main(int argc, char **argv)
{
	int showusage = 0;
	CFHD_Error error = CFHD_ERROR_OKAY;

	setbuf(stdout, NULL);

	if (argc != 2)
	{
		showusage = 1;
	}
	else if (argv[1][0] == '-')
	{
		if (argv[1][1] == 'd' || argv[1][1] == 'D')
			error = EncodeDecodeQualityTest();
		else if (argv[1][1] == 'e' || argv[1][1] == 'E')
			error = EncodeSpeedTest();
		else
			showusage = 1;
	}
	else 
	{
		char ext[4] = "";
		int len = strlen(argv[1]);
		if (len > 4)
		{
			ext[0] = argv[1][len - 3];
			ext[1] = argv[1][len - 2];
			ext[2] = argv[1][len - 1];
			ext[3] = 0;
		}

		error = DecodeMOVIE(argv[1], ext);
	}

	if (showusage)
	{
		printf("usage: %s [switches] or <filename.MOV|MP4|AVI>\n", argv[0]);
		printf("          -D = decoder tester\n");
		printf("          -E = encoder tester\n");
	}

	if (error) printf("error code: %d\n", error);
	return error;
}
