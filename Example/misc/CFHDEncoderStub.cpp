// $Header$

#include "StdAfx.h"
#include "../../Includes/CFHDEncoder.h"

//TODO: Eliminate use of unsafe functions
#pragma warning(disable: 4996)


#if DYNAMICALLY_LINK


// Open an instance of the CineForm HD ENCODER
typedef CFHD_Error (*lpCFHD_OpenEncoder)(CFHD_EncoderRef *encoderRefOut, CFHD_ALLOCATOR *allocator);
// Return a list of input formats in decreasing order of preference
typedef CFHD_Error (*lpCFHD_GetInputFormats)(CFHD_EncoderRef encoderRef,
	CFHD_PixelFormat *inputFormatArray,
	int inputFormatArrayLength,
	int *actualInputFormatCountOut);
// Initialize for encoding frames with the specified dimensions and format
typedef CFHD_Error (*lpCFHD_PrepareToEncode)(CFHD_EncoderRef encoderRef,
	int frameWidth,
	int frameHeight,
	CFHD_PixelFormat pixelFormat,
	CFHD_EncodedFormat encodedFormat,
	CFHD_EncodingFlags encodingFlags,
	CFHD_EncodingQuality encodingQuality);

// Set the license for the encoder, controlling time trials and encode resolutions, else watermarked
typedef CFHD_Error (*lpCFHD_SetEncodeLicense)(CFHD_EncoderRef encoderRef, unsigned char *licenseKey);
// Set the license for the encoder, controlling time trials and encode resolutions, else watermarked
typedef CFHD_Error (*lpCFHD_SetEncodeLicense2)(CFHD_EncoderRef encoderRef, unsigned char *licenseKey, uint32_t *level);

// Encode one sample of CineForm HD
typedef CFHD_Error (*lpCFHD_EncodeSample)(CFHD_EncoderRef encoderRef,
	void *frameBuffer,
	int framePitch);
// Get the sample data and size of the encoded sample
typedef CFHD_Error (*lpCFHD_GetSampleData)(CFHD_EncoderRef encoderRef,
	void **sampleDataOut,
	size_t *sampleSizeOut);
// Close an instance of the CineForm HD decoder
typedef CFHD_Error (*lpCFHD_CloseEncoder)(CFHD_EncoderRef encoderRef);
typedef CFHD_Error (*lpCFHD_GetEncodeThumbnail)(CFHD_EncoderRef encoderRef,
	void *samplePtr,
	size_t sampleSize,
	void *outputBuffer,
	size_t outputBufferSize,
	uint32_t flags,
	size_t *retWidth,
	size_t *retHeight,
	size_t *retSize);
typedef CFHD_Error (*lpCFHD_MetadataOpen)(CFHD_MetadataRef *metadataRefOut);
typedef CFHD_Error (*lpCFHD_MetadataAdd)(CFHD_MetadataRef metadataRef,
	uint32_t tag,
	CFHD_MetadataType type, 
	size_t size,
	uint32_t *data,
	bool temporary);	
typedef CFHD_Error (*lpCFHD_MetadataAttach)(CFHD_EncoderRef encoderRef, CFHD_MetadataRef metadataRef);
typedef CFHD_Error (*lpCFHD_MetadataClose)(CFHD_MetadataRef metadataRef);
typedef void (*lpCFHD_ApplyWatermark)(void *frameBuffer,
	int frameWidth,
	int frameHeight,
	int framePitch,
	CFHD_PixelFormat pixelFormat);
// Create an encoder pool for asynchronous encoding
typedef CFHD_Error (*lpCFHD_CreateEncoderPool)(CFHD_EncoderPoolRef *encoderPoolRefOut,
	int encoderThreadCount,
	int jobQueueLength,
	CFHD_ALLOCATOR *allocator);
// Return a list of input formats in decreasing order of preference
typedef CFHD_Error (*lpCFHD_GetAsyncInputFormats)(CFHD_EncoderPoolRef encoderPoolRef,
	CFHD_PixelFormat *inputFormatArray,
	int inputFormatArrayLength,
	int *actualInputFormatCountOut);
// Prepare the asynchronous encoders in a pool for encoding
typedef CFHD_Error (*lpCFHD_PrepareEncoderPool)(CFHD_EncoderPoolRef encoderPoolRef,
	uint_least16_t frameWidth,
	uint_least16_t frameHeight,
	CFHD_PixelFormat pixelFormat,
	CFHD_EncodedFormat encodedFormat,
	CFHD_EncodingFlags encodingFlags,
	CFHD_EncodingQuality encodingQuality);

// Set the license for all of the encoders in the pool (otherwise use watermark)
typedef CFHD_Error (*lpCFHD_SetEncoderPoolLicense)(CFHD_EncoderPoolRef encoderPoolRef, unsigned char *licenseKey);
// Set the license for all of the encoders in the pool (otherwise use watermark)
typedef CFHD_Error (*lpCFHD_SetEncoderPoolLicense2)(CFHD_EncoderPoolRef encoderPoolRef, unsigned char *licenseKey, uint32_t *level);

// Attach metadata to all of the encoders in the pool
typedef CFHD_Error (*lpCFHD_AttachEncoderPoolMetadata)(CFHD_EncoderPoolRef encoderPoolRef,
	CFHD_MetadataRef metadataRef);
// Start the asynchronous encoders
typedef CFHD_Error (*lpCFHD_StartEncoderPool)(CFHD_EncoderPoolRef encoderPoolRef);
// Stop the asynchronous encoders
typedef CFHD_Error (*lpCFHD_StopEncoderPool)(CFHD_EncoderPoolRef encoderPoolRef);
// Submit a frame for asynchronous encoding
typedef CFHD_Error (*lpCFHD_EncodeAsyncSample)(CFHD_EncoderPoolRef encoderPoolRef,
	uint32_t frameNumber,
	void *frameBuffer,
	intptr_t framePitch,
	CFHD_MetadataRef metadataRef);
// Wait until the next encoded sample is ready
typedef CFHD_Error (*lpCFHD_WaitForSample)(CFHD_EncoderPoolRef encoderPoolRef,
	uint32_t *frameNumberOut,
	CFHD_SampleBufferRef *sampleBufferRefOut);
// Test whether the next encoded sample is ready
typedef CFHD_Error (*lpCFHD_TestForSample)(CFHD_EncoderPoolRef encoderPoolRef,
	uint32_t *frameNumberOut,
	CFHD_SampleBufferRef *sampleBufferRefOut);
// Get the size and address of an encoded sample
typedef CFHD_Error (*lpCFHD_GetEncodedSample)(CFHD_SampleBufferRef sampleBufferRef,
	void **sampleDataOut,
	size_t *sampleSizeOut);
// Get the thumbnail image from an encoded sample
typedef CFHD_Error (*lpCFHD_GetSampleThumbnail)(CFHD_SampleBufferRef sampleBufferRef,
	void *thumbnailBuffer,
	size_t bufferSize,
	uint32_t flags,
	uint_least16_t *actualWidthOut,
	uint_least16_t *actualHeightOut,
	CFHD_PixelFormat *pixelFormatOut,
	size_t *actualSizeOut);
// Release the sample buffer
typedef CFHD_Error (*lpCFHD_ReleaseSampleBuffer)(CFHD_EncoderPoolRef encoderPoolRef,
	CFHD_SampleBufferRef sampleBufferRef);
// Release the encoder pool
typedef CFHD_Error (*lpCFHD_ReleaseEncoderPool)(CFHD_EncoderPoolRef encoderPoolRef);



lpCFHD_OpenEncoder					CF_OpenEncoder;
lpCFHD_GetInputFormats				CF_GetInputFormats; 
lpCFHD_PrepareToEncode				CF_PrepareToEncode;
lpCFHD_SetEncodeLicense				CF_SetEncodeLicense; 
lpCFHD_SetEncodeLicense2			CF_SetEncodeLicense2; 
lpCFHD_EncodeSample					CF_EncodeSample; 
lpCFHD_GetSampleData				CF_GetSampleData; 
lpCFHD_CloseEncoder					CF_CloseEncoder; 
lpCFHD_GetEncodeThumbnail			CF_GetEncodeThumbnail; 
lpCFHD_MetadataOpen					CF_MetadataOpen; 
lpCFHD_MetadataAdd					CF_MetadataAdd; 
lpCFHD_MetadataAttach				CF_MetadataAttach; 
lpCFHD_MetadataClose				CF_MetadataClose; 
lpCFHD_ApplyWatermark				CF_ApplyWatermark; 
lpCFHD_CreateEncoderPool			CF_CreateEncoderPool; 
lpCFHD_GetAsyncInputFormats			CF_GetAsyncInputFormats; 
lpCFHD_PrepareEncoderPool			CF_PrepareEncoderPool; 
lpCFHD_SetEncoderPoolLicense		CF_SetEncoderPoolLicense; 
lpCFHD_SetEncoderPoolLicense2		CF_SetEncoderPoolLicense2; 
lpCFHD_AttachEncoderPoolMetadata	CF_AttachEncoderPoolMetadata; 
lpCFHD_StartEncoderPool				CF_StartEncoderPool; 
lpCFHD_StopEncoderPool				CF_StopEncoderPool; 
lpCFHD_EncodeAsyncSample			CF_EncodeAsyncSample; 
lpCFHD_WaitForSample				CF_WaitForSample; 
lpCFHD_TestForSample				CF_TestForSample; 
lpCFHD_GetEncodedSample				CF_GetEncodedSample; 
lpCFHD_GetSampleThumbnail			CF_GetSampleThumbnail; 
lpCFHD_ReleaseSampleBuffer			CF_ReleaseSampleBuffer; 
lpCFHD_ReleaseEncoderPool			CF_ReleaseEncoderPool; 


static void *pLib = NULL;

#if __APPLE__
#pragma warning disable 556
void * getDLLEntry( void *pLib, const char * entryName)
{
	void * temp = dlsym(pLib, entryName);
	char * error;
	
	if(temp == NULL) {
		if(error=dlerror()) {
			fprintf(stderr, "%s: not found error:%s\n",entryName, error);
		} else {
			fprintf(stderr, "%s: not found in dlyb\n", entryName);
		}
	}
	return temp;
}
#endif

CFHD_Error LoadDLL()
{
#if __APPLE__
	char DLLPath[260] = "/library/Application Support/CineForm/Libs/libCFHDEncoder.dylib";
	pLib = dlopen(DLLPath, RTLD_NOW);

	if(pLib)
	{
		CF_OpenEncoder = (lpCFHD_OpenEncoder)getDLLEntry(pLib, "CFHD_OpenEncoder");
		CF_GetInputFormats = (lpCFHD_GetInputFormats)getDLLEntry(pLib, "CFHD_GetInputFormats");
		CF_PrepareToEncode = (lpCFHD_PrepareToEncode)getDLLEntry(pLib, "CFHD_PrepareToEncode");
		CF_SetEncodeLicense = (lpCFHD_SetEncodeLicense)getDLLEntry(pLib, "CFHD_SetEncodeLicense");
		CF_SetEncodeLicense2 = (lpCFHD_SetEncodeLicense2)getDLLEntry(pLib, "CFHD_SetEncodeLicense2");
		CF_EncodeSample = (lpCFHD_EncodeSample)getDLLEntry(pLib, "CFHD_EncodeSample");
		CF_GetSampleData = (lpCFHD_GetSampleData)getDLLEntry(pLib, "CFHD_GetSampleData");
		CF_CloseEncoder = (lpCFHD_CloseEncoder)getDLLEntry(pLib, "CFHD_CloseEncoder");
		CF_GetEncodeThumbnail = (lpCFHD_GetEncodeThumbnail)getDLLEntry(pLib, "CFHD_GetEncodeThumbnail");
		CF_MetadataOpen = (lpCFHD_MetadataOpen)getDLLEntry(pLib, "CFHD_MetadataOpen");
		CF_MetadataAdd = (lpCFHD_MetadataAdd)getDLLEntry(pLib, "CFHD_MetadataAdd");
		CF_MetadataAttach = (lpCFHD_MetadataAttach)getDLLEntry(pLib, "CFHD_MetadataAttach");
		CF_MetadataClose = (lpCFHD_MetadataClose)getDLLEntry(pLib, "CFHD_MetadataClose");
		CF_ApplyWatermark = (lpCFHD_ApplyWatermark)getDLLEntry(pLib, "CFHD_ApplyWatermark");
		CF_CreateEncoderPool = (lpCFHD_CreateEncoderPool)getDLLEntry(pLib, "CFHD_CreateEncoderPool");
		CF_GetAsyncInputFormats = (lpCFHD_GetAsyncInputFormats)getDLLEntry(pLib, "CFHD_GetAsyncInputFormats");
		CF_PrepareEncoderPool = (lpCFHD_PrepareEncoderPool)getDLLEntry(pLib, "CFHD_PrepareEncoderPool");
		CF_SetEncoderPoolLicense = (lpCFHD_SetEncoderPoolLicense)getDLLEntry(pLib, "CFHD_SetEncoderPoolLicense");
		CF_SetEncoderPoolLicense2 = (lpCFHD_SetEncoderPoolLicense2)getDLLEntry(pLib, "CFHD_SetEncoderPoolLicense2");
		CF_AttachEncoderPoolMetadata = (lpCFHD_AttachEncoderPoolMetadata)getDLLEntry(pLib, "CFHD_AttachEncoderPoolMetadata");
		CF_StartEncoderPool = (lpCFHD_StartEncoderPool)getDLLEntry(pLib, "CFHD_StartEncoderPool");
		CF_StopEncoderPool = (lpCFHD_StopEncoderPool)getDLLEntry(pLib, "CFHD_StopEncoderPool");
		CF_EncodeAsyncSample = (lpCFHD_EncodeAsyncSample)getDLLEntry(pLib, "CFHD_EncodeAsyncSample");
		CF_WaitForSample = (lpCFHD_WaitForSample)getDLLEntry(pLib, "CFHD_WaitForSample");
		CF_TestForSample = (lpCFHD_TestForSample)getDLLEntry(pLib, "CFHD_TestForSample");
		CF_GetEncodedSample = (lpCFHD_GetEncodedSample)getDLLEntry(pLib, "CFHD_GetEncodedSample");
		CF_GetSampleThumbnail = (lpCFHD_GetSampleThumbnail)getDLLEntry(pLib, "CFHD_GetSampleThumbnail");
		CF_ReleaseSampleBuffer = (lpCFHD_ReleaseSampleBuffer)getDLLEntry(pLib, "CFHD_ReleaseSampleBuffer");
		CF_ReleaseEncoderPool = (lpCFHD_ReleaseEncoderPool)getDLLEntry(pLib, "CFHD_ReleaseEncoderPool");

		if(CF_SetEncodeLicense2 == NULL)
			CF_SetEncodeLicense2 = (lpCFHD_SetEncodeLicense2)CFHD_SetEncodeLicenseCompat;
		if(CF_SetEncoderPoolLicense2 == NULL)
			CF_SetEncoderPoolLicense2 = (lpCFHD_SetEncoderPoolLicense2)CFHD_SetEncoderPoolLicenseCompat;

		if(!(	CF_OpenEncoder						 &&
				CF_GetInputFormats					 &&
				CF_PrepareToEncode					 &&
				CF_SetEncodeLicense					 &&
				CF_EncodeSample						 &&
				CF_GetSampleData					 &&
				CF_CloseEncoder						 &&
				CF_GetEncodeThumbnail				 &&
				CF_MetadataOpen						 &&
				CF_MetadataAdd						 &&
				CF_MetadataAttach					 &&
				CF_MetadataClose					 &&
				CF_ApplyWatermark					 &&
				CF_CreateEncoderPool				 &&
				CF_GetAsyncInputFormats				 &&
				CF_PrepareEncoderPool				 &&
				CF_SetEncoderPoolLicense			 &&
				CF_AttachEncoderPoolMetadata		 &&
				CF_StartEncoderPool					 &&
				CF_StopEncoderPool					 &&
				CF_EncodeAsyncSample				 &&
				CF_WaitForSample					 &&
				CF_TestForSample					 &&
				CF_GetEncodedSample					 &&
				CF_GetSampleThumbnail				 &&
				CF_ReleaseSampleBuffer				 &&
				CF_ReleaseEncoderPool		
				) )			 
		{
			printf("Encoder DLL too old.\n");
			return CFHD_ERROR_UNEXPECTED;
		}
	}
	else
	{
		printf("failed to open libCFHDEncoder.dylib.\n");
		return CFHD_ERROR_UNEXPECTED;
	}
#endif

#if _WIN32
	DWORD retVal;
	char DLLPath[260] = "C:\\Program Files (x86)\\CineForm\\Tools";

	HKEY lmSWKey = NULL;
	HKEY wowKey = NULL;
	HKEY wowCFKey = NULL;
	HKEY isKey = NULL;
	retVal = RegOpenKeyEx(HKEY_LOCAL_MACHINE, "SOFTWARE", 0, KEY_ALL_ACCESS, &lmSWKey);
	if(retVal == ERROR_SUCCESS)
	{
		retVal = RegOpenKeyEx(lmSWKey, "CineForm", 0, KEY_ALL_ACCESS, &wowCFKey);
		if(retVal != ERROR_SUCCESS)
		{
			retVal = RegOpenKeyEx(lmSWKey, "Wow6432Node", 0, KEY_ALL_ACCESS, &wowKey);
			if(retVal == ERROR_SUCCESS)
			{
				retVal = RegOpenKeyEx(wowKey, "CineForm", 0, KEY_ALL_ACCESS, &wowCFKey);
			}
		}

		if(retVal == ERROR_SUCCESS)
		{
			retVal = RegOpenKeyEx(wowCFKey, "InstallSpots", 0, KEY_ALL_ACCESS, &isKey);
			if(retVal == ERROR_SUCCESS)
			{
				char flPath [MAX_PATH] = {'\0'};
				DWORD strSize = MAX_PATH;
				retVal = RegGetValue(isKey, NULL, "CineFormTools", RRF_RT_REG_SZ, NULL, flPath, &strSize);
				if(retVal == ERROR_SUCCESS)
				{
					strncpy(DLLPath, flPath, 259);
				}
			}
		}
	}

	if(lmSWKey)
		RegCloseKey(lmSWKey);
	if(wowKey)
		RegCloseKey(wowKey);
	if(wowCFKey)
		RegCloseKey(wowCFKey);
	if(isKey)
		RegCloseKey(isKey);


	#ifdef _WIN64
	strcat(DLLPath, "\\CFHDEncoder64.dll");
	#else
	strcat(DLLPath, "\\CFHDEncoder.dll");
	#endif
	pLib = (void*)LoadLibrary(DLLPath);

	if(pLib == NULL) // Try opening a local copy
	{
		#ifdef _WIN64
		pLib = (void*)LoadLibrary("CFHDEncoder64.dll");
		#else
		pLib = (void*)LoadLibrary("CFHDEncoder.dll");
		#endif
	}

	if(pLib)
	{
		CF_OpenEncoder = (lpCFHD_OpenEncoder)GetProcAddress((HMODULE)pLib, "CFHD_OpenEncoder");
		CF_GetInputFormats = (lpCFHD_GetInputFormats)GetProcAddress((HMODULE)pLib, "CFHD_GetInputFormats");
		CF_PrepareToEncode = (lpCFHD_PrepareToEncode)GetProcAddress((HMODULE)pLib, "CFHD_PrepareToEncode");
		CF_SetEncodeLicense = (lpCFHD_SetEncodeLicense)GetProcAddress((HMODULE)pLib, "CFHD_SetEncodeLicense");
		CF_SetEncodeLicense2 = (lpCFHD_SetEncodeLicense2)GetProcAddress((HMODULE)pLib, "CFHD_SetEncodeLicense2");
		CF_EncodeSample = (lpCFHD_EncodeSample)GetProcAddress((HMODULE)pLib, "CFHD_EncodeSample");
		CF_GetSampleData = (lpCFHD_GetSampleData)GetProcAddress((HMODULE)pLib, "CFHD_GetSampleData");
		CF_CloseEncoder = (lpCFHD_CloseEncoder)GetProcAddress((HMODULE)pLib, "CFHD_CloseEncoder");
		CF_GetEncodeThumbnail = (lpCFHD_GetEncodeThumbnail)GetProcAddress((HMODULE)pLib, "CFHD_GetEncodeThumbnail");
		CF_MetadataOpen = (lpCFHD_MetadataOpen)GetProcAddress((HMODULE)pLib, "CFHD_MetadataOpen");
		CF_MetadataAdd = (lpCFHD_MetadataAdd)GetProcAddress((HMODULE)pLib, "CFHD_MetadataAdd");
		CF_MetadataAttach = (lpCFHD_MetadataAttach)GetProcAddress((HMODULE)pLib, "CFHD_MetadataAttach");
		CF_MetadataClose = (lpCFHD_MetadataClose)GetProcAddress((HMODULE)pLib, "CFHD_MetadataClose");
		CF_ApplyWatermark = (lpCFHD_ApplyWatermark)GetProcAddress((HMODULE)pLib, "CFHD_ApplyWatermark");
		CF_CreateEncoderPool = (lpCFHD_CreateEncoderPool)GetProcAddress((HMODULE)pLib, "CFHD_CreateEncoderPool");
		CF_GetAsyncInputFormats = (lpCFHD_GetAsyncInputFormats)GetProcAddress((HMODULE)pLib, "CFHD_GetAsyncInputFormats");
		CF_PrepareEncoderPool = (lpCFHD_PrepareEncoderPool)GetProcAddress((HMODULE)pLib, "CFHD_PrepareEncoderPool");
		CF_SetEncoderPoolLicense = (lpCFHD_SetEncoderPoolLicense)GetProcAddress((HMODULE)pLib, "CFHD_SetEncoderPoolLicense");
		CF_SetEncoderPoolLicense2 = (lpCFHD_SetEncoderPoolLicense2)GetProcAddress((HMODULE)pLib, "CFHD_SetEncoderPoolLicense2");
		CF_AttachEncoderPoolMetadata = (lpCFHD_AttachEncoderPoolMetadata)GetProcAddress((HMODULE)pLib, "CFHD_AttachEncoderPoolMetadata");
		CF_StartEncoderPool = (lpCFHD_StartEncoderPool)GetProcAddress((HMODULE)pLib, "CFHD_StartEncoderPool");
		CF_StopEncoderPool = (lpCFHD_StopEncoderPool)GetProcAddress((HMODULE)pLib, "CFHD_StopEncoderPool");
		CF_EncodeAsyncSample = (lpCFHD_EncodeAsyncSample)GetProcAddress((HMODULE)pLib, "CFHD_EncodeAsyncSample");
		CF_WaitForSample = (lpCFHD_WaitForSample)GetProcAddress((HMODULE)pLib, "CFHD_WaitForSample");
		CF_TestForSample = (lpCFHD_TestForSample)GetProcAddress((HMODULE)pLib, "CFHD_TestForSample");
		CF_GetEncodedSample = (lpCFHD_GetEncodedSample)GetProcAddress((HMODULE)pLib, "CFHD_GetEncodedSample");
		CF_GetSampleThumbnail = (lpCFHD_GetSampleThumbnail)GetProcAddress((HMODULE)pLib, "CFHD_GetSampleThumbnail");
		CF_ReleaseSampleBuffer = (lpCFHD_ReleaseSampleBuffer)GetProcAddress((HMODULE)pLib, "CFHD_ReleaseSampleBuffer");
		CF_ReleaseEncoderPool = (lpCFHD_ReleaseEncoderPool)GetProcAddress((HMODULE)pLib, "CFHD_ReleaseEncoderPool");

		if(CF_SetEncodeLicense2 == NULL)
			CF_SetEncodeLicense2 = (lpCFHD_SetEncodeLicense2)CFHD_SetEncodeLicenseCompat;
		if(CF_SetEncoderPoolLicense2 == NULL)
			CF_SetEncoderPoolLicense2 = (lpCFHD_SetEncoderPoolLicense2)CFHD_SetEncoderPoolLicenseCompat;

		if(!(	CF_OpenEncoder						 &&
				CF_GetInputFormats					 &&
				CF_PrepareToEncode					 &&
				CF_SetEncodeLicense					 &&
				CF_EncodeSample						 &&
				CF_GetSampleData					 &&
				CF_CloseEncoder						 &&
				CF_GetEncodeThumbnail				 &&
				CF_MetadataOpen						 &&
				CF_MetadataAdd						 &&
				CF_MetadataAttach					 &&
				CF_MetadataClose					 &&
				CF_ApplyWatermark					 &&
				CF_CreateEncoderPool				 &&
				CF_GetAsyncInputFormats				 &&
				CF_PrepareEncoderPool				 &&
				CF_SetEncoderPoolLicense			 &&
				CF_AttachEncoderPoolMetadata		 &&
				CF_StartEncoderPool					 &&
				CF_StopEncoderPool					 &&
				CF_EncodeAsyncSample				 &&
				CF_WaitForSample					 &&
				CF_TestForSample					 &&
				CF_GetEncodedSample					 &&
				CF_GetSampleThumbnail				 &&
				CF_ReleaseSampleBuffer				 &&
				CF_ReleaseEncoderPool		 
				) )		
		{
			printf("Decoder DLL too old.\n");
			return CFHD_ERROR_UNEXPECTED;
		}
	}
	else
	{
		#ifdef _WIN64
		printf("failed to open CFHDDecoder64.dll.\n");
		#else
		printf("failed to open CFHDDecoder.dll.\n");
		#endif
		return CFHD_ERROR_UNEXPECTED;
	}
#endif

	return CFHD_ERROR_OKAY;
}


CFHD_Error CFHD_OpenEncoderStub(CFHD_EncoderRef *encoderRefOut,
				 CFHD_ALLOCATOR *allocator)
{				 
	if(pLib == NULL)
		LoadDLL();
	if(pLib == NULL)
		return CFHD_ERROR_UNEXPECTED;
	return CF_OpenEncoder(encoderRefOut, allocator);
}

// Return a list of input formats in decreasing order of preference
CFHD_Error CFHD_GetInputFormatsStub(CFHD_EncoderRef encoderRef,
					 CFHD_PixelFormat *inputFormatArray,
					 int inputFormatArrayLength,
					 int *actualInputFormatCountOut)
{
	if(pLib == NULL)
		return CFHD_ERROR_UNEXPECTED;
	return CF_GetInputFormats(encoderRef,
					 inputFormatArray,
					 inputFormatArrayLength,
					 actualInputFormatCountOut);
}

// Initialize for encoding frames with the specified dimensions and format
CFHD_Error CFHD_PrepareToEncodeStub(CFHD_EncoderRef encoderRef,
					 int frameWidth,
					 int frameHeight,
					 CFHD_PixelFormat pixelFormat,
					 CFHD_EncodedFormat encodedFormat,
					 CFHD_EncodingFlags encodingFlags,
					 CFHD_EncodingQuality encodingQuality)
{
	if(pLib == NULL)
		return CFHD_ERROR_UNEXPECTED;
	return CF_PrepareToEncode(encoderRef,
					 frameWidth,
					 frameHeight,
					 pixelFormat,
					 encodedFormat,
					 encodingFlags,
					 encodingQuality);
}

// Set the license for the encoder, controlling time trials and encode resolutions, else watermarked
CFHD_Error CFHD_SetEncodeLicenseStub(CFHD_EncoderRef encoderRef,
					  unsigned char *licenseKey)
{
	if(pLib == NULL)
		return CFHD_ERROR_UNEXPECTED;
	return CF_SetEncodeLicense(encoderRef, licenseKey);
}
// Set the license for the encoder, controlling time trials and encode resolutions, else watermarked
CFHD_Error CFHD_SetEncodeLicense2Stub(CFHD_EncoderRef encoderRef,
					  unsigned char *licenseKey, uint32_t *level)
{
	if(pLib == NULL)
		return CFHD_ERROR_UNEXPECTED;
	return CF_SetEncodeLicense2(encoderRef, licenseKey, level);
}
// Set the license for the encoder, controlling time trials and encode resolutions, else watermarked
CFHD_Error CFHD_SetEncodeLicenseCompat(CFHD_EncoderRef encoderRef,
					  unsigned char *licenseKey, uint32_t *level)
{
	CFHD_Error err;
	*level = 0;
	if(pLib == NULL)
		return CFHD_ERROR_UNEXPECTED;

	err = CF_SetEncodeLicense(encoderRef, licenseKey);
	if(err == CFHD_ERROR_OKAY)
		*level = 0xffffffff;

	return err;
}

// Encode one sample of CineForm HD
CFHD_Error CFHD_EncodeSampleStub(CFHD_EncoderRef encoderRef,
				  void *frameBuffer,
				  int framePitch)
{
	if(pLib == NULL)
		return CFHD_ERROR_UNEXPECTED;
	return CF_EncodeSample(encoderRef, frameBuffer, framePitch);
}

// Get the sample data and size of the encoded sample
CFHD_Error CFHD_GetSampleDataStub(CFHD_EncoderRef encoderRef,
				   void **sampleDataOut,
				   size_t *sampleSizeOut)
{
	if(pLib == NULL)
		return CFHD_ERROR_UNEXPECTED;
	return CF_GetSampleData(encoderRef, sampleDataOut, sampleSizeOut);
}

// Close an instance of the CineForm HD decoder
CFHD_Error CFHD_CloseEncoderStub(CFHD_EncoderRef encoderRef)
{
	if(pLib == NULL)
		return CFHD_ERROR_UNEXPECTED;
	return CF_CloseEncoder(encoderRef);
}


CFHD_Error CFHD_GetEncodeThumbnailStub(CFHD_EncoderRef encoderRef,
						void *samplePtr,
						size_t sampleSize,
						void *outputBuffer,
						size_t outputBufferSize,
						uint32_t flags,
						size_t *retWidth,
						size_t *retHeight,
						size_t *retSize)
{
	if(pLib == NULL)
		return CFHD_ERROR_UNEXPECTED;
	return CF_GetEncodeThumbnail(encoderRef,
						samplePtr,
						sampleSize,
						outputBuffer,
						outputBufferSize,
						flags,
						retWidth,
						retHeight,
						retSize);
}

CFHD_Error CFHD_MetadataOpenStub(CFHD_MetadataRef *metadataRefOut)
{
	if(pLib == NULL)
		return CFHD_ERROR_UNEXPECTED;
	return CF_MetadataOpen(metadataRefOut);
}

CFHD_Error CFHD_MetadataAddStub(CFHD_MetadataRef metadataRef,
				 uint32_t tag,
				 CFHD_MetadataType type, 
				 size_t size,
				 uint32_t *data,
				 bool temporary)
{
	if(pLib == NULL)
		return CFHD_ERROR_UNEXPECTED;
	return CF_MetadataAdd(metadataRef,
				 tag,
				 type, 
				 size,
				 data,
				 temporary);
}
	
CFHD_Error CFHD_MetadataAttachStub(CFHD_EncoderRef encoderRef, CFHD_MetadataRef metadataRef)
{
	if(pLib == NULL)
		return CFHD_ERROR_UNEXPECTED;
	return CF_MetadataAttach(encoderRef, metadataRef);
}

CFHD_Error CFHD_MetadataCloseStub(CFHD_MetadataRef metadataRef)
{
	if(pLib == NULL)
		return CFHD_ERROR_UNEXPECTED;
	return CF_MetadataClose(metadataRef);
}

void CFHD_ApplyWatermarkStub(void *frameBuffer,
					int frameWidth,
					int frameHeight,
					int framePitch,
					CFHD_PixelFormat pixelFormat)
{
	if(pLib == NULL)
		return;
	return CF_ApplyWatermark(frameBuffer,
					frameWidth,
					frameHeight,
					framePitch,
					pixelFormat);
}

// Create an encoder pool for asynchronous encoding
CFHD_Error CFHD_CreateEncoderPoolStub(CFHD_EncoderPoolRef *encoderPoolRefOut,
					   int encoderThreadCount,
					   int jobQueueLength,
					   CFHD_ALLOCATOR *allocator)
{
	if(pLib == NULL)
		LoadDLL();
	if(pLib == NULL)
		return CFHD_ERROR_UNEXPECTED;
	return CF_CreateEncoderPool(encoderPoolRefOut,
					   encoderThreadCount,
					   jobQueueLength,
					   allocator);
}

// Return a list of input formats in decreasing order of preference
CFHD_Error CFHD_GetAsyncInputFormatsStub(CFHD_EncoderPoolRef encoderPoolRef,
						  CFHD_PixelFormat *inputFormatArray,
						  int inputFormatArrayLength,
						  int *actualInputFormatCountOut)
{
	if(pLib == NULL)
		return CFHD_ERROR_UNEXPECTED;
	return CF_GetAsyncInputFormats(encoderPoolRef,
						  inputFormatArray,
						  inputFormatArrayLength,
						  actualInputFormatCountOut);
}

// Prepare the asynchronous encoders in a pool for encoding
CFHD_Error CFHD_PrepareEncoderPoolStub(CFHD_EncoderPoolRef encoderPoolRef,
						uint_least16_t frameWidth,
						uint_least16_t frameHeight,
						CFHD_PixelFormat pixelFormat,
						CFHD_EncodedFormat encodedFormat,
						CFHD_EncodingFlags encodingFlags,
						CFHD_EncodingQuality encodingQuality)
{
	if(pLib == NULL)
		return CFHD_ERROR_UNEXPECTED;
	return CF_PrepareEncoderPool(encoderPoolRef,
						frameWidth,
						frameHeight,
						pixelFormat,
						encodedFormat,
						encodingFlags,
						encodingQuality);
}

// Set the license for all of the encoders in the pool (otherwise use watermark)
CFHD_Error CFHD_SetEncoderPoolLicenseStub(CFHD_EncoderPoolRef encoderPoolRef,
						   unsigned char *licenseKey)
{
	if(pLib == NULL)
		return CFHD_ERROR_UNEXPECTED;
	return CF_SetEncoderPoolLicense(encoderPoolRef, licenseKey);
}
// Set the license for all of the encoders in the pool (otherwise use watermark)
CFHD_Error CFHD_SetEncoderPoolLicense2Stub(CFHD_EncoderPoolRef encoderPoolRef,
						   unsigned char *licenseKey, uint32_t *level)
{
	if(pLib == NULL)
		return CFHD_ERROR_UNEXPECTED;
	return CF_SetEncoderPoolLicense2(encoderPoolRef, licenseKey, level);
}
// Set the license for all of the encoders in the pool (otherwise use watermark)
CFHD_Error CFHD_SetEncoderPoolLicenseCompat(CFHD_EncoderPoolRef encoderPoolRef,
						   unsigned char *licenseKey, uint32_t *level)
{
	CFHD_Error err;
	*level = 0;
	if(pLib == NULL)
		return CFHD_ERROR_UNEXPECTED;
	err = CF_SetEncoderPoolLicense(encoderPoolRef, licenseKey);

	if(err == CFHD_ERROR_OKAY)
		*level = 0xffffffff;

	return err;
}

// Attach metadata to all of the encoders in the pool
CFHD_Error CFHD_AttachEncoderPoolMetadataStub(CFHD_EncoderPoolRef encoderPoolRef,
							   CFHD_MetadataRef metadataRef)
{
	if(pLib == NULL)
		return CFHD_ERROR_UNEXPECTED;
	return CF_AttachEncoderPoolMetadata(encoderPoolRef, metadataRef);
}

// Start the asynchronous encoders
CFHD_Error CFHD_StartEncoderPoolStub(CFHD_EncoderPoolRef encoderPoolRef)
{
	if(pLib == NULL)
		return CFHD_ERROR_UNEXPECTED;
	return CF_StartEncoderPool(encoderPoolRef);
}

// Stop the asynchronous encoders
CFHD_Error CFHD_StopEncoderPoolStub(CFHD_EncoderPoolRef encoderPoolRef)
{
	if(pLib == NULL)
		return CFHD_ERROR_UNEXPECTED;
	return CF_StopEncoderPool(encoderPoolRef);
}

// Submit a frame for asynchronous encoding
CFHD_Error CFHD_EncodeAsyncSampleStub(CFHD_EncoderPoolRef encoderPoolRef,
					   uint32_t frameNumber,
					   void *frameBuffer,
					   intptr_t framePitch,
					   CFHD_MetadataRef metadataRef)
{
	if(pLib == NULL)
		return CFHD_ERROR_UNEXPECTED;
	return CF_EncodeAsyncSample(encoderPoolRef,
					   frameNumber,
					   frameBuffer,
					   framePitch,
					   metadataRef);
}

// Wait until the next encoded sample is ready
CFHD_Error CFHD_WaitForSampleStub(CFHD_EncoderPoolRef encoderPoolRef,
				   uint32_t *frameNumberOut,
				   CFHD_SampleBufferRef *sampleBufferRefOut)
{
	if(pLib == NULL)
		return CFHD_ERROR_UNEXPECTED;
	return CF_WaitForSample(encoderPoolRef,
				   frameNumberOut,
				   sampleBufferRefOut);
}

// Test whether the next encoded sample is ready
CFHD_Error CFHD_TestForSampleStub(CFHD_EncoderPoolRef encoderPoolRef,
				   uint32_t *frameNumberOut,
				   CFHD_SampleBufferRef *sampleBufferRefOut)
{
	if(pLib == NULL)
		return CFHD_ERROR_UNEXPECTED;
	return CF_TestForSample(encoderPoolRef,
				   frameNumberOut,
				   sampleBufferRefOut);
}

// Get the size and address of an encoded sample
CFHD_Error CFHD_GetEncodedSampleStub(CFHD_SampleBufferRef sampleBufferRef,
					  void **sampleDataOut,
					  size_t *sampleSizeOut)
{
	if(pLib == NULL)
		return CFHD_ERROR_UNEXPECTED;
	return CF_GetEncodedSample(sampleBufferRef,
					  sampleDataOut,
					  sampleSizeOut);
}

// Get the thumbnail image from an encoded sample
CFHD_Error CFHD_GetSampleThumbnailStub(CFHD_SampleBufferRef sampleBufferRef,
						void *thumbnailBuffer,
						size_t bufferSize,
						uint32_t flags,
						uint_least16_t *actualWidthOut,
						uint_least16_t *actualHeightOut,
						CFHD_PixelFormat *pixelFormatOut,
						size_t *actualSizeOut)
{
	if(pLib == NULL)
		return CFHD_ERROR_UNEXPECTED;
	return CF_GetSampleThumbnail(sampleBufferRef,
						thumbnailBuffer,
						bufferSize,
						flags,
						actualWidthOut,
						actualHeightOut,
						pixelFormatOut,
						actualSizeOut);
}

// Release the sample buffer
CFHD_Error CFHD_ReleaseSampleBufferStub(CFHD_EncoderPoolRef encoderPoolRef,
						 CFHD_SampleBufferRef sampleBufferRef)
{
	if(pLib == NULL)
		return CFHD_ERROR_UNEXPECTED;
	return CF_ReleaseSampleBuffer(encoderPoolRef, sampleBufferRef);
}

// Release the encoder pool
CFHD_Error CFHD_ReleaseEncoderPoolStub(CFHD_EncoderPoolRef encoderPoolRef)
{
	if(pLib == NULL)
		return CFHD_ERROR_UNEXPECTED;
	return CF_ReleaseEncoderPool(encoderPoolRef);
}


#endif