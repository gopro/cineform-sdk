// $Header$

#pragma once

// Opaque data type for a DPX file reader
typedef void *DPXFileReaderRef;

#ifdef __cplusplus
extern "C" {
#endif

// Open a DPX file and read the header information
CFHD_Error DPXFileOpen(DPXFileReaderRef *fileRef,
					   const char *pathnameTemplate,
					   int frameNumber);

// Read a frame from the DPX file
CFHD_Error DPXReadFrame(DPXFileReaderRef fileRef,
						void *frameBuffer,
						int framePitch,
						CFHD_PixelFormat pixelFormat);

// Return the frame dimensions from the DPX file header
int DPXFrameWidth(DPXFileReaderRef fileRef);
int DPXFrameHeight(DPXFileReaderRef fileRef);

// Return an array of pixel formats supported by the DPX file reader
CFHD_Error DPXGetPixelFormats(DPXFileReaderRef fileRef,
							  CFHD_PixelFormat *pixelFormatArray,
							  int pixelFormatArrayLength,
							  int *actualPixelFormatCountOut);

// Return the preferred frame pitch for the specified pixel format
int DPXFramePitch(DPXFileReaderRef fileRef,
				  CFHD_PixelFormat pixelFormat);

// Close the DPX file
CFHD_Error DPXFileReaderClose(DPXFileReaderRef fileRef);

#ifdef __cplusplus
}
#endif
