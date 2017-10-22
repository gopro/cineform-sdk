// $Header$

#include "StdAfx.h"
#include "CFHDDecoder.h"
#include "CFHDMetadata.h"

//TODO: Eliminate use of unsafe string functions
#pragma warning(disable: 4996)

#if DYNAMICALLY_LINK

typedef CFHD_Error (*lpCFHD_OpenDecoder)(CFHD_DecoderRef *decoderRefOut,
	 CFHD_ALLOCATOR *allocator);
typedef CFHD_Error (*lpCFHD_CloseDecoder)(CFHD_DecoderRef decoderRef);
typedef CFHD_Error (*lpCFHD_PrepareToDecode)(CFHD_DecoderRef decoderRef,
	int outputWidth,
	int outputHeight,
	CFHD_PixelFormat outputFormat,
	CFHD_DecodedResolution decodedResolution,
	CFHD_DecodingFlags decodingFlags,
	void *samplePtr,
	size_t sampleSize,
	int *actualWidthOut,
	int *actualHeightOut,
	CFHD_PixelFormat *actualFormatOut);
typedef CFHD_Error (*lpCFHD_DecodeSample)(CFHD_DecoderRef decoderRef,
	void *samplePtr,
	size_t sampleSize,
	void *outputBuffer,
	int outputPitch);
typedef CFHD_Error (*lpCFHD_GetOutputFormats)(CFHD_DecoderRef decoderRef,
	void *samplePtr,
	size_t sampleSize,
	CFHD_PixelFormat *outputFormatArray,
	int outputFormatArrayLength,
	int *actualOutputFormatCountOut);

typedef CFHD_Error (*lpCFHD_OpenMetadata)(CFHD_MetadataRef *metadataRefOut);
typedef CFHD_Error (*lpCFHD_CloseMetadata)(CFHD_MetadataRef metadataRef);
typedef CFHD_Error (*lpCFHD_InitSampleMetadata)(CFHD_MetadataRef metadataRef,
	CFHD_MetadataTrack track, 
	void *sampleData,
	size_t sampleSize);
typedef CFHD_Error (*lpCFHD_ReadMetadata)(CFHD_MetadataRef metadataRef,
	unsigned int *tag, 
	CFHD_MetadataType *type, 
	void **data, 
	CFHD_MetadataSize *size);
typedef CFHD_Error (*lpCFHD_FindMetadata)(CFHD_MetadataRef metadataRef,
	unsigned int tag, 
	CFHD_MetadataType *type, 
	void **data, 
	CFHD_MetadataSize *size);

typedef CFHD_Error (*lpCFHD_SetActiveMetadata)(CFHD_DecoderRef decoderRef,
	CFHD_MetadataRef metadataRef,
	unsigned int tag, 
	CFHD_MetadataType type, 
	void *data, 
	CFHD_MetadataSize size);
typedef CFHD_Error (*lpCFHD_SetLicense)(CFHD_DecoderRef decoderRef,
	const unsigned char *licenseKey);
typedef CFHD_Error (*lpCFHD_GetThumbnail)(CFHD_DecoderRef decoderRef,
	void *samplePtr,
	size_t sampleSize,
	void *outputBuffer,
	size_t outputBufferSize,
	uint32_t flags,
	size_t *retWidth,
	size_t *retHeight,
	size_t *retSize);
typedef CFHD_Error (*lpCFHD_GetSampleInfo)(CFHD_DecoderRef decoderRef,
	void *samplePtr,
	size_t sampleSize,
	CFHD_SampleInfoTag tag,
	void *value,
	size_t buffer_size);
typedef CFHD_Error (*lpCFHD_GetPixelSize)(CFHD_PixelFormat pixelFormat, uint32_t *pixelSizeOut);
typedef CFHD_Error (*lpCFHD_GetImagePitch)(uint32_t imageWidth, CFHD_PixelFormat pixelFormat, int32_t *imagePitchOut);
typedef CFHD_Error (*lpCFHD_GetImageSize)(uint32_t imageWidth, uint32_t imageHeight, CFHD_PixelFormat pixelFormat, 
										  CFHD_VideoSelect videoselect,	CFHD_Stereo3DType stereotype, uint32_t *imageSizeOut);

typedef CFHD_Error (*lpCFHD_ClearActiveMetadata)(CFHD_DecoderRef decoderRef, CFHD_MetadataRef metadataRef);

lpCFHD_OpenDecoder CF_OpenDecoder;
lpCFHD_CloseDecoder CF_CloseDecoder;
lpCFHD_PrepareToDecode CF_PrepareToDecode;
lpCFHD_DecodeSample CF_DecodeSample;
lpCFHD_GetOutputFormats CF_GetOutputFormats;
lpCFHD_OpenMetadata CF_OpenMetadata;
lpCFHD_CloseMetadata CF_CloseMetadata;
lpCFHD_InitSampleMetadata CF_InitSampleMetadata;
lpCFHD_ReadMetadata CF_ReadMetadata;
lpCFHD_FindMetadata CF_FindMetadata;
lpCFHD_SetActiveMetadata CF_SetActiveMetadata;
lpCFHD_SetLicense CF_SetLicense;
lpCFHD_GetThumbnail CF_GetThumbnail;
lpCFHD_GetSampleInfo CF_GetSampleInfo;
lpCFHD_GetPixelSize CF_GetPixelSize;
lpCFHD_GetImageSize CF_GetImageSize;
lpCFHD_GetImagePitch CF_GetImagePitch;
lpCFHD_ClearActiveMetadata CF_ClearActiveMetadata;

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
	char DLLPath[260] = "/library/Application Support/CineForm/Libs/libCFHDDecoder.dylib";
	pLib = dlopen(DLLPath, RTLD_NOW);

	if(pLib)
	{
		CF_OpenDecoder = (lpCFHD_OpenDecoder)getDLLEntry(pLib, "CFHD_OpenDecoder");
		CF_CloseDecoder = (lpCFHD_CloseDecoder)getDLLEntry(pLib, "CFHD_CloseDecoder");
		CF_PrepareToDecode = (lpCFHD_PrepareToDecode)getDLLEntry(pLib, "CFHD_PrepareToDecode");
		CF_DecodeSample = (lpCFHD_DecodeSample)getDLLEntry(pLib, "CFHD_DecodeSample");
		CF_GetOutputFormats = (lpCFHD_GetOutputFormats)getDLLEntry(pLib, "CFHD_GetOutputFormats");
		CF_OpenMetadata = (lpCFHD_OpenMetadata)getDLLEntry(pLib, "CFHD_OpenMetadata");
		CF_CloseMetadata = (lpCFHD_CloseMetadata)getDLLEntry(pLib, "CFHD_CloseMetadata");
		CF_InitSampleMetadata = (lpCFHD_InitSampleMetadata)getDLLEntry(pLib, "CFHD_InitSampleMetadata");
		CF_ReadMetadata = (lpCFHD_ReadMetadata)getDLLEntry(pLib, "CFHD_ReadMetadata");
		CF_FindMetadata = (lpCFHD_FindMetadata)getDLLEntry(pLib, "CFHD_FindMetadata");
		CF_SetActiveMetadata = (lpCFHD_SetActiveMetadata)getDLLEntry(pLib, "CFHD_SetActiveMetadata");
		CF_SetLicense = (lpCFHD_SetLicense)getDLLEntry(pLib, "CFHD_SetLicense");
		CF_GetThumbnail = (lpCFHD_GetThumbnail)getDLLEntry(pLib, "CFHD_GetThumbnail");
		CF_GetSampleInfo = (lpCFHD_GetSampleInfo)getDLLEntry(pLib, "CFHD_GetSampleInfo");
		CF_GetPixelSize = (lpCFHD_GetPixelSize)getDLLEntry(pLib, "CFHD_GetPixelSize");
		CF_GetImageSize = (lpCFHD_GetImageSize)getDLLEntry(pLib, "CFHD_GetImageSize");
		CF_GetImagePitch = (lpCFHD_GetImagePitch)getDLLEntry(pLib, "CFHD_GetImagePitch");
		CF_ClearActiveMetadata = (lpCFHD_ClearActiveMetadata)getDLLEntry(pLib, "CFHD_ClearActiveMetadata");

		if(!(	CF_OpenDecoder && 
				CF_CloseDecoder &&
				CF_PrepareToDecode &&
				CF_DecodeSample &&
				CF_GetOutputFormats  &&
				CF_OpenMetadata &&
				CF_CloseMetadata &&
				CF_InitSampleMetadata &&
				CF_ReadMetadata &&
				CF_FindMetadata &&
				CF_SetActiveMetadata &&
				CF_SetLicense &&
				CF_GetThumbnail &&
				CF_GetSampleInfo &&
				CF_GetPixelSize &&
				CF_GetImageSize &&
				CF_GetImagePitch &&
				CF_ClearActiveMetadata) )
		{
			printf("Decoder DLL too old.\n");
			return CFHD_ERROR_UNEXPECTED;
		}
	}
	else
	{
		printf("failed to open libCFHDDecoder.dyllib.\n");
		return CFHD_ERROR_UNEXPECTED;
	}
#endif

#if _WINDOWS
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
	strcat(DLLPath, "\\CFHDDecoder64.dll");
	#else
	strcat(DLLPath, "\\CFHDDecoder.dll");
	#endif
	pLib = (void*)LoadLibrary(DLLPath);

	if(pLib == NULL) // Try opening a local copy
	{
		#ifdef _WIN64
		pLib = (void*)LoadLibrary("CFHDDecoder64.dll");
		#else
		pLib = (void*)LoadLibrary("CFHDDecoder.dll");
		#endif
	}

	if(pLib)
	{
		CF_OpenDecoder = (lpCFHD_OpenDecoder)GetProcAddress((HMODULE)pLib, "CFHD_OpenDecoder");
		CF_CloseDecoder = (lpCFHD_CloseDecoder)GetProcAddress((HMODULE)pLib, "CFHD_CloseDecoder");
		CF_PrepareToDecode = (lpCFHD_PrepareToDecode)GetProcAddress((HMODULE)pLib, "CFHD_PrepareToDecode");
		CF_DecodeSample = (lpCFHD_DecodeSample)GetProcAddress((HMODULE)pLib, "CFHD_DecodeSample");
		CF_GetOutputFormats = (lpCFHD_GetOutputFormats)GetProcAddress((HMODULE)pLib, "CFHD_GetOutputFormats");
		CF_OpenMetadata = (lpCFHD_OpenMetadata)GetProcAddress((HMODULE)pLib, "CFHD_OpenMetadata");
		CF_CloseMetadata = (lpCFHD_CloseMetadata)GetProcAddress((HMODULE)pLib, "CFHD_CloseMetadata");
		CF_InitSampleMetadata = (lpCFHD_InitSampleMetadata)GetProcAddress((HMODULE)pLib, "CFHD_InitSampleMetadata");
		CF_ReadMetadata = (lpCFHD_ReadMetadata)GetProcAddress((HMODULE)pLib, "CFHD_ReadMetadata");
		CF_FindMetadata = (lpCFHD_FindMetadata)GetProcAddress((HMODULE)pLib, "CFHD_FindMetadata");
		CF_SetActiveMetadata = (lpCFHD_SetActiveMetadata)GetProcAddress((HMODULE)pLib, "CFHD_SetActiveMetadata");
		CF_SetLicense = (lpCFHD_SetLicense)GetProcAddress((HMODULE)pLib, "CFHD_SetLicense");
		CF_GetThumbnail = (lpCFHD_GetThumbnail)GetProcAddress((HMODULE)pLib, "CFHD_GetThumbnail");
		CF_GetSampleInfo = (lpCFHD_GetSampleInfo)GetProcAddress((HMODULE)pLib, "CFHD_GetSampleInfo");
		CF_GetPixelSize = (lpCFHD_GetPixelSize)GetProcAddress((HMODULE)pLib, "CFHD_GetPixelSize");
		CF_GetImageSize = (lpCFHD_GetImageSize)GetProcAddress((HMODULE)pLib, "CFHD_GetImageSize");
		CF_GetImagePitch = (lpCFHD_GetImagePitch)GetProcAddress((HMODULE)pLib, "CFHD_GetImagePitch");
		CF_ClearActiveMetadata = (lpCFHD_ClearActiveMetadata)GetProcAddress((HMODULE)pLib, "CFHD_ClearActiveMetadata");

		if(!(	CF_OpenDecoder && 
				CF_CloseDecoder &&
				CF_PrepareToDecode &&
				CF_DecodeSample &&
				CF_GetOutputFormats  &&
				CF_OpenMetadata &&
				CF_CloseMetadata &&
				CF_InitSampleMetadata &&
				CF_ReadMetadata &&
				CF_FindMetadata &&
				CF_SetActiveMetadata &&
				CF_SetLicense &&
				CF_GetThumbnail &&
				CF_GetSampleInfo &&
				CF_GetPixelSize &&
				CF_GetImageSize &&
				CF_GetImagePitch &&
				CF_ClearActiveMetadata) )
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




CFHD_Error CFHD_OpenDecoderStub(
	CFHD_DecoderRef *decoderRefOut,
	CFHD_ALLOCATOR *allocator)
{
	if(pLib == NULL)
		LoadDLL();
	if(pLib == NULL)
		return CFHD_ERROR_UNEXPECTED;
	return CF_OpenDecoder(decoderRefOut, allocator);
}

CFHD_Error CFHD_CloseDecoderStub(CFHD_DecoderRef decoderRef)
{
	if(pLib == NULL)
		return CFHD_ERROR_UNEXPECTED;
	return CF_CloseDecoder(decoderRef);
}

CFHD_Error CFHD_PrepareToDecodeStub(CFHD_DecoderRef decoderRef,
	int outputWidth,
	int outputHeight,
	CFHD_PixelFormat outputFormat,
	CFHD_DecodedResolution decodedResolution,
	CFHD_DecodingFlags decodingFlags,
	void *samplePtr,
	size_t sampleSize,
	int *actualWidthOut,
	int *actualHeightOut,
	CFHD_PixelFormat *actualFormatOut)
{
	if(pLib == NULL)
		return CFHD_ERROR_UNEXPECTED;
	return CF_PrepareToDecode(
	 decoderRef,
	 outputWidth,
	 outputHeight,
	 outputFormat,
	 decodedResolution,
	 decodingFlags,
	 samplePtr,
	 sampleSize,
	 actualWidthOut,
	 actualHeightOut,
	 actualFormatOut);
}

CFHD_Error CFHD_DecodeSampleStub(CFHD_DecoderRef decoderRef,
	void *samplePtr,
	size_t sampleSize,
	void *outputBuffer,
	int outputPitch)
{
	if(pLib == NULL)
		return CFHD_ERROR_UNEXPECTED;
	return CF_DecodeSample(
		decoderRef,
		samplePtr,
		sampleSize,
		outputBuffer,
		outputPitch);
}

CFHD_Error CFHD_GetOutputFormatsStub(CFHD_DecoderRef decoderRef,
	void *samplePtr,
	size_t sampleSize,
	CFHD_PixelFormat *outputFormatArray,
	int outputFormatArrayLength,
	int *actualOutputFormatCountOut)
{
	if(pLib == NULL)
		return CFHD_ERROR_UNEXPECTED;
	return CF_GetOutputFormats(
		decoderRef,
		samplePtr,
		sampleSize,
		outputFormatArray,
		outputFormatArrayLength,
		actualOutputFormatCountOut);
}

CFHD_Error CFHD_OpenMetadataStub(CFHD_MetadataRef *metadataRefOut)
{
	if(pLib == NULL)
		LoadDLL();
	if(pLib == NULL)
		return CFHD_ERROR_UNEXPECTED;
	return CF_OpenMetadata(metadataRefOut);
}

CFHD_Error CFHD_CloseMetadataStub(CFHD_MetadataRef metadataRef)
{
	if(pLib == NULL)
		return CFHD_ERROR_UNEXPECTED;
	return CF_CloseMetadata(metadataRef);
}

CFHD_Error CFHD_InitSampleMetadataStub(CFHD_MetadataRef metadataRef,
	CFHD_MetadataTrack track, 
	void *sampleData,
	size_t sampleSize)
{
	if(pLib == NULL)
		return CFHD_ERROR_UNEXPECTED;
	return CF_InitSampleMetadata(
		metadataRef,
		track, 
		sampleData,
		sampleSize);
}

CFHD_Error CFHD_ReadMetadataStub(CFHD_MetadataRef metadataRef,
	unsigned int *tag, 
	CFHD_MetadataType *type, 
	void **data, 
	CFHD_MetadataSize *size)
{
	if(pLib == NULL)
		return CFHD_ERROR_UNEXPECTED;
	return CF_ReadMetadata(
		metadataRef,
		tag, 
		type, 
		data, 
		size);
}

CFHD_Error CFHD_FindMetadataStub(CFHD_MetadataRef metadataRef,
	unsigned int tag, 
	CFHD_MetadataType *type, 
	void **data, 
	CFHD_MetadataSize *size)
{
	if(pLib == NULL)
		return CFHD_ERROR_UNEXPECTED;
	return CF_FindMetadata(
		metadataRef,
		tag, 
		type, 
		data, 
		size);
}

CFHD_Error CFHD_SetActiveMetadataStub(CFHD_DecoderRef decoderRef,
	CFHD_MetadataRef metadataRef,
	unsigned int tag, 
	CFHD_MetadataType type, 
	void *data, 
	CFHD_MetadataSize size)
{
	if(pLib == NULL)
		return CFHD_ERROR_UNEXPECTED;
	return CF_SetActiveMetadata(
		decoderRef,
		metadataRef,
		tag, 
		type, 
		data, 
		size);
}

CFHD_Error CFHD_SetLicenseStub(CFHD_DecoderRef decoderRef,
	const unsigned char *licenseKey)
{
	if(pLib == NULL)
		return CFHD_ERROR_UNEXPECTED;
	return CF_SetLicense(
		decoderRef,
		licenseKey);
}

CFHD_Error CFHD_GetThumbnailStub(CFHD_DecoderRef decoderRef,
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
	return CF_GetThumbnail(
		decoderRef,
		samplePtr,
		sampleSize,
		outputBuffer,
		outputBufferSize,
		flags,
		retWidth,
		retHeight,
		retSize);
}

CFHD_Error CFHD_GetSampleInfoStub(CFHD_DecoderRef decoderRef,
	void *samplePtr,
	size_t sampleSize,
	CFHD_SampleInfoTag tag,
	void *value,
	size_t buffer_size)
{
	if(pLib == NULL)
		return CFHD_ERROR_UNEXPECTED;
	return CF_GetSampleInfo(
		decoderRef,
		samplePtr,
		sampleSize,
		tag,
		value,
		buffer_size);
}



CFHD_Error CFHD_GetPixelSizeStub(CFHD_PixelFormat pixelFormat, uint32_t *pixelSizeOut)
{
	if(pLib == NULL)
		return CFHD_ERROR_UNEXPECTED;
	return CF_GetPixelSize(pixelFormat, pixelSizeOut);
}

CFHD_Error CFHD_GetImagePitchStub(uint32_t imageWidth, CFHD_PixelFormat pixelFormat, int32_t *imagePitchOut)
{
	if(pLib == NULL)
		return CFHD_ERROR_UNEXPECTED;
	return CF_GetImagePitch(imageWidth, pixelFormat, imagePitchOut);
}

CFHD_Error CFHD_GetImageSizeStub(uint32_t imageWidth, uint32_t imageHeight, CFHD_PixelFormat pixelFormat, 
								 CFHD_VideoSelect videoselect,	CFHD_Stereo3DType stereotype, uint32_t *imageSizeOut)
{
	if(pLib == NULL)
		return CFHD_ERROR_UNEXPECTED;
	return CF_GetImageSize(imageWidth, imageHeight, pixelFormat, videoselect, stereotype, imageSizeOut);
}


CFHD_Error CFHD_ClearActiveMetadata(CFHD_DecoderRef decoderRef, CFHD_MetadataRef metadataRef)
{
	if(pLib == NULL)
		return CFHD_ERROR_UNEXPECTED;
	return CF_ClearActiveMetadata(
		decoderRef,
		metadataRef);
}


#endif