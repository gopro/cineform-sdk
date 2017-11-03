// $Header$

/*! @file DPXWriter.cpp
    *
	*  @brief Routines for writing the images decoded by CineForm SDK to DPX files. 
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

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <math.h>
#include <limits.h>
#include <malloc.h>
#include <assert.h>
#include "CFHDDecoder.h"
#include "CFHDMetadata.h"
#include "DPXWriter.h"

#define CGRGB 1		//0 studioRGB, 1 computer graphics RGB

#if _WINDOWS

#define Swap16(x)	_byteswap_ushort(x)
#define Swap32(x)	_byteswap_ulong(x)
#define Swap32f(x)	(float)_byteswap_ulong((uint32_t)x)

#else

// Use the byte swapping functions provided by GCC
#include <byteswap.h>
#define Swap16(x)	bswap_16(x)
#define Swap32(x)	bswap_32(x)
#define Swap32f(x)	(float)bswap_32((uint32_t)(x))

#endif

#if _WINDOWS && !defined(PATH_MAX)
#define PATH_MAX _MAX_PATH
#endif

//TODO: Eliminates the cause of warnings about unsafe functions
#pragma warning(disable: 4996)

static size_t RoundUpBytes(size_t byteCount)
{
	// DPX sizes are rounded to a 32-bit word
	return ((byteCount + 3) & ~((size_t)0x03));
}

uint32_t Pack10(int red, int green, int blue)
{
	// Reduce each 16 bit color value to 10 bits
	const int shift = 6;

	const int red10 = 22;
	const int green10 = 12;
	const int blue10 = 2;

	const uint32_t mask10 = 0x3FF;

	red >>= shift;
	green >>= shift;
	blue >>= shift;

	// Pack the color values into a 32 bit word
	uint32_t word = ((red & mask10) << red10) | ((green &mask10) << green10) | ((blue & mask10) << blue10);

	// Return the packed word after byte swapping (if necessary)
	return Swap32(word);
}


// Define a smart buffer for a row of packed DPX pixels
class CRowBuffer
{
public:

	CRowBuffer(size_t size) :
		m_buffer(NULL)
	{
		m_buffer = malloc(size);
	}

	~CRowBuffer()
	{
		if (m_buffer) {
			free(m_buffer);
			m_buffer = NULL;
		}
	}

	operator uint32_t * () {
		return (uint32_t *)m_buffer;
	}

private:

	void *m_buffer;

};


class CDPXFile
{
public:

	// Cineon DPX file format: http://www.cineon.com/ff_draft.php

	// Data types used in the Cineon DPX file format specification
	typedef unsigned char U8;
	typedef unsigned short U16;
	typedef uint32_t U32;
	typedef long S32;
	typedef float R32;
	typedef char ASCII;

	static const U32 SPDX = 0x53445058;
	static const U32 XPDS = 0x58504453;

	enum PixelFormat
	{
		kPixelFormat_RGB = 50,
	};

	CDPXFile(int imageWidth, int imageHeight);

	~CDPXFile()
	{
		if (m_file != NULL) {
			fclose(m_file);
			m_file = NULL;
		}
	}

	// Open the DPX file and write the file headers
	bool Create(const char *filename);

	// Write the next row of pixels
	bool WriteRow(uint32_t *rowPtr, size_t rowSize);

private:

	// Data structures copied from the Cineon DPX file format specification
	typedef struct file_information
	{
		U32   magic_num;        /* magic number 0x53445058 (SDPX) or 0x58504453 (XPDS) */
		U32   offset;           /* offset to image data in bytes */
		ASCII vers[8];          /* which header format version is being used (v1.0)*/
		U32   file_size;        /* file size in bytes */
		U32   ditto_key;        /* read time short cut - 0 = same, 1 = new */
		U32   gen_hdr_size;     /* generic header length in bytes */
		U32   ind_hdr_size;     /* industry header length in bytes */
		U32   user_data_size;   /* user-defined data length in bytes */
		ASCII file_name[100];   /* iamge file name */
		ASCII create_time[24];  /* file creation date "yyyy:mm:dd:hh:mm:ss:LTZ" */
		ASCII creator[100];     /* file creator's name */
		ASCII project[200];     /* project name */
		ASCII copyright[200];   /* right to use or copyright info */
		U32   key;              /* encryption ( FFFFFFFF = unencrypted ) */
		ASCII Reserved[104];    /* reserved field TBD (need to pad) */
	} FileInformation;

	typedef struct _image_information
	{
		U16    orientation;          /* image orientation */
		U16    element_number;       /* number of image elements */
		U32    pixels_per_line;      /* or x value */
		U32    lines_per_image_ele;  /* or y value, per element */
		struct _image_element
		{
			U32    data_sign;        /* data sign (0 = unsigned, 1 = signed ) */
									 /* "Core set images are unsigned" */
			U32    ref_low_data;     /* reference low data code value */
			R32    ref_low_quantity; /* reference low quantity represented */
			U32    ref_high_data;    /* reference high data code value */
			R32    ref_high_quantity;/* reference high quantity represented */
			U8     descriptor;       /* descriptor for image element */
			U8     transfer;         /* transfer characteristics for element */
			U8     colorimetric;     /* colormetric specification for element */
			U8     bit_size;         /* bit size for element */
			U16    packing;          /* packing for element */
			U16    encoding;         /* encoding for element */
			U32    data_offset;      /* offset to data of element */
			U32    eol_padding;      /* end of line padding used in element */
			U32    eo_image_padding; /* end of image padding used in element */
			ASCII  description[32];  /* description of element */
		} image_element[8];          /* NOTE THERE ARE EIGHT OF THESE */

		U8 reserved[52];             /* reserved for future use (padding) */
	} Image_Information;

	typedef struct _image_orientation
	{
		U32   x_offset;               /* X offset */
		U32   y_offset;               /* Y offset */
		R32   x_center;               /* X center */
		R32   y_center;               /* Y center */
		U32   x_orig_size;            /* X original size */
		U32   y_orig_size;            /* Y original size */
		ASCII file_name[100];         /* source image file name */
		ASCII creation_time[24];      /* source image creation date and time */
		ASCII input_dev[32];          /* input device name */
		ASCII input_serial[32];       /* input device serial number */
		U16   border[4];              /* border validity (XL, XR, YT, YB) */
		U32   pixel_aspect[2];        /* pixel aspect ratio (H:V) */
		U8    reserved[28];           /* reserved for future use (padding) */
	} Image_Orientation;

	// Film and motion picture header (industry specific header)
	typedef struct _motion_picture_film_header
	{
		ASCII film_mfg_id[2];    /* film manufacturer ID code (2 digits from film edge code) */
		ASCII film_type[2];      /* file type (2 digits from film edge code) */
		ASCII offset[2];         /* offset in perfs (2 digits from film edge code)*/
		ASCII prefix[6];         /* prefix (6 digits from film edge code) */
		ASCII count[4];          /* count (4 digits from film edge code)*/
		ASCII format[32];        /* format (i.e. academy) */
		U32   frame_position;    /* frame position in sequence */
		U32   sequence_len;      /* sequence length in frames */
		U32   held_count;        /* held count (1 = default) */
		R32   frame_rate;        /* frame rate of original in frames/sec */
		R32   shutter_angle;     /* shutter angle of camera in degrees */
		ASCII frame_id[32];      /* frame identification (i.e. keyframe) */
		ASCII slate_info[100];   /* slate information */
		U8    reserved[56];      /* reserved for future use (padding) */
	} Motion_Picture_Film;

	// Television header (industry specific header)
	typedef struct _television_header
	{
		U32 tim_code;            /* SMPTE time code */
		U32 userBits;            /* SMPTE user bits */
		U8  interlace;           /* interlace ( 0 = noninterlaced, 1 = 2:1 interlace*/
		U8  field_num;           /* field number */
		U8  video_signal;        /* video signal standard (table 4)*/
		U8  unused;              /* used for byte alignment only */
		R32 hor_sample_rate;     /* horizontal sampling rate in Hz */
		R32 ver_sample_rate;     /* vertical sampling rate in Hz */
		R32 frame_rate;          /* temporal sampling rate or frame rate in Hz */
		R32 time_offset;         /* time offset from sync to first pixel */
		R32 gamma;               /* gamma value */
		R32 black_level;         /* black level code value */
		R32 black_gain;          /* black gain */
		R32 break_point;         /* breakpoint */
		R32 white_level;         /* reference white level code value */
		R32 integration_times;   /* integration time(s) */
		U8  reserved[76];        /* reserved for future use (padding) */
	} Television_Header;

	// This structure is not defined in the specification
	typedef struct _pixel_aspect_ratio
	{
		U32 horizontal;
		U32 vertical;
	} Pixel_Aspect_Ratio;

protected:

	// POSIX file descriptor for writing the DPX file
	FILE *m_file;

	// File information block
	FileInformation m_fileInfo;

	// Image information block
	Image_Information m_imageInfo;

	// Image orientation block
	Image_Orientation m_imageHeader;

	// Motion picture film industry header (optional)
	Motion_Picture_Film m_filmHeader;

	// Television industry header (optional)
	Television_Header m_videoHeader;

	// Offset to the image data (in bytes)
	size_t m_imageOffset;

	// Image dimensions
	size_t m_imageWidth;
	size_t m_imageHeight;

	// Low and high reference data
	U32 m_refLowData;
	R32 m_refLowQuantity;
	U32 m_refHighData;
	R32 m_refHighQuantity;

	// Pixel format information
	int m_bitsPerPixel;
	int m_pixelPacking;
	int m_pixelEncoding;
	int m_pixelFormat;		// See DPX File Structure Table 1

	size_t m_pixelSize;		// Size of all pixel components (in bytes)

	size_t m_bytesPerRow;	// Image row pitch (in bytes)

	int m_dittoKey;			// Read time short cut
	int m_dataOffset;		// Offset to data in the image element

	// Pixel aspect ratio
	Pixel_Aspect_Ratio m_pixelAspectRatio;

	// Frame rate from the video header (if present)
	float m_videoFrameRate;

	// Gamma value from the video header (if present)
	float m_videoGamma;
};

CDPXFile::CDPXFile(int imageWidth, int imageHeight) :
	m_file(NULL)
{
	m_imageWidth = imageWidth;
	m_imageHeight = imageHeight;

	m_pixelEncoding = 0;
	m_pixelFormat = 50;
	m_pixelSize = 4;
	m_bytesPerRow = (m_imageWidth * m_pixelSize);

	m_pixelAspectRatio.horizontal = ULONG_MAX;
	m_pixelAspectRatio.vertical = ULONG_MAX;

	// Fill in the file parameters with values for writing a 10-bit DPX file
	m_imageOffset = 2048;
	m_bitsPerPixel = 10;
	m_pixelPacking = 1;
	m_pixelEncoding = 0;
	m_pixelFormat = kPixelFormat_RGB;
	m_pixelSize = 4;

	m_dittoKey = 1;

	m_refLowData = 0;
	m_refLowQuantity = 0.0f;
	m_refHighData = 1023;
	m_refHighQuantity = 0.0f;

	m_dataOffset = 2048;

	// Compute the output pitch
	m_bytesPerRow = RoundUpBytes(m_imageWidth * m_pixelSize);

	m_pixelAspectRatio.horizontal = ULONG_MAX;
	m_pixelAspectRatio.vertical = ULONG_MAX;
}

bool CDPXFile::Create(const char *filename)
{
	size_t writeCount;

	// Try to open the DPX file in binary mode
	m_file = fopen(filename, "wb");
	if (!m_file) {
		return false;
	}

	// Use unbuffered writes
	setbuf(m_file, NULL);

	size_t genericHeaderSize = sizeof(m_fileInfo) + sizeof(m_imageInfo) + sizeof(m_imageHeader);
	size_t industryHeaderSize = sizeof(m_filmHeader) + sizeof(m_videoHeader);
	size_t totalHeaderSize = genericHeaderSize + industryHeaderSize;
	size_t imageSize = m_imageWidth * m_imageHeight * m_pixelSize;
	size_t fileSize = imageSize + totalHeaderSize;
	assert(totalHeaderSize == 2048);

	// Initialize the file information header
	memset(&m_fileInfo, 0, sizeof(m_fileInfo));

	// Set the magic number to indicate that byte swapping is required
	m_fileInfo.magic_num = XPDS;

	m_fileInfo.offset = Swap32((U32)totalHeaderSize);
	strcpy(m_fileInfo.vers, "V1.0");
	m_fileInfo.file_size = Swap32((U32)fileSize);
	m_fileInfo.ditto_key = Swap32(m_dittoKey);
	m_fileInfo.gen_hdr_size = Swap32((U32)genericHeaderSize);
	m_fileInfo.ind_hdr_size = Swap32((U32)industryHeaderSize);

	m_fileInfo.key = 0xFFFFFFFF;

	// Write the file information header
	writeCount = fwrite(&m_fileInfo, sizeof(m_fileInfo), 1, m_file);
	assert(writeCount == 1);

	// Initialize the image information header
	memset(&m_imageInfo, 0, sizeof(m_imageInfo));
	m_imageInfo.orientation = 0;
	m_imageInfo.element_number = Swap16(1);
	m_imageInfo.pixels_per_line = Swap32((U32)m_imageWidth);
	m_imageInfo.lines_per_image_ele = Swap32((U32)m_imageHeight);

	// Initialize the members of the first image element
	m_imageInfo.image_element[0].data_sign = 0;
	m_imageInfo.image_element[0].ref_low_data = Swap32(m_refLowData);
	m_imageInfo.image_element[0].ref_low_quantity = Swap32f(m_refLowQuantity);
	m_imageInfo.image_element[0].ref_high_data = Swap32(m_refHighData);
	m_imageInfo.image_element[0].ref_high_quantity = Swap32f(m_refHighQuantity);
	m_imageInfo.image_element[0].descriptor = 50;
	m_imageInfo.image_element[0].bit_size = m_bitsPerPixel;
	m_imageInfo.image_element[0].packing = Swap16(1);
	m_imageInfo.image_element[0].data_offset = Swap32(m_dataOffset);

	// Write the image information header
	writeCount = fwrite(&m_imageInfo, sizeof(m_imageInfo), 1, m_file);
	assert(writeCount == 1);

	// Initialize the image orientation header
	memset(&m_imageHeader, 0, sizeof(m_imageHeader));
	m_imageHeader.pixel_aspect[0] = ULONG_MAX;
	m_imageHeader.pixel_aspect[1] = ULONG_MAX;

	// Write the image orientation header
	writeCount = fwrite(&m_imageHeader, sizeof(m_imageHeader), 1, m_file);
	assert(writeCount == 1);

	// Initialize the film and motion picture industry header
	memset(&m_filmHeader, 0, sizeof(m_filmHeader));

	// Write the film and motion picture industry header
	writeCount = fwrite(&m_filmHeader, sizeof(m_filmHeader), 1, m_file);
	assert(writeCount == 1);

	// Initialize the television industry header
	memset(&m_videoHeader, 0, sizeof(m_videoHeader));

	// Write the television industry header
	writeCount = fwrite(&m_videoHeader, sizeof(m_videoHeader), 1, m_file);
	assert(writeCount == 1);

	return true;
}

bool CDPXFile::WriteRow(uint32_t *bufferPtr, size_t bufferSize)
{
	size_t writeCount;
	writeCount = fwrite(bufferPtr, bufferSize, 1, m_file);
	assert(writeCount == 1);

	return true;
}


// Entry points for the simple DPX file writer

/*!
	@function WriteToDPX

	@brief Writes an image buffer to a DPX file.

	@description The DPX file format written by the routine is packed
	10-bit RGB.  The pathname template must contain a decimal format
	specifier for the frame number (for example, %04d).

	@bug The image buffers decoded by DecoderDLL may have more than
	10 bits of precision, but the extra precision is discarded.

	@return No return value.
*/
void WriteToDPX(char *pathnameTemplate,		//! Format string with a decimal format specifier
				int frameNumber,			//! Frame number
				uint16_t *imageBuffer,		//! Buffer of Bayer pixel data
				int bufferWidth,			//! Width of buffer in values (not Bayer quads)
				int bufferHeight,			//! Height of buffer in values (not Bayer quads)
				int bufferPitch,			//! Number of bytes per row
				int bufferFormat,			//! Pixel format
				int bayerFormat				//! Bayer format (if the pixel format is Bayer)
				)
{
	switch (bufferFormat)
	{
	case CFHD_PIXEL_FORMAT_B64A:
		WriteARGB64(pathnameTemplate, frameNumber, imageBuffer, bufferWidth, bufferHeight, bufferPitch);
		break;

	case CFHD_PIXEL_FORMAT_RG48:
		WriteRG48(pathnameTemplate, frameNumber, imageBuffer, bufferWidth, bufferHeight, bufferPitch);
		break;

	case CFHD_PIXEL_FORMAT_WP13:
		WriteWP13(pathnameTemplate, frameNumber, imageBuffer, bufferWidth, bufferHeight, bufferPitch);
		break;

	case CFHD_PIXEL_FORMAT_W13A:
		WriteW13A(pathnameTemplate, frameNumber, imageBuffer, bufferWidth, bufferHeight, bufferPitch);
		break;

	case CFHD_PIXEL_FORMAT_BGRA:
		WriteARGB32(pathnameTemplate, frameNumber, imageBuffer, bufferWidth, bufferHeight, bufferPitch);
		break;

	case CFHD_PIXEL_FORMAT_RG24:
		WriteRGB24(pathnameTemplate, frameNumber, imageBuffer, bufferWidth, bufferHeight, bufferPitch);
		break;

	case CFHD_PIXEL_FORMAT_R408: 
		WriteR408(pathnameTemplate, frameNumber, imageBuffer, bufferWidth, bufferHeight, bufferPitch);
		break;
	case CFHD_PIXEL_FORMAT_V408: 
		WriteV408(pathnameTemplate, frameNumber, imageBuffer, bufferWidth, bufferHeight, bufferPitch);
		break;

	case CFHD_PIXEL_FORMAT_DPX0:
		WriteDPX0(pathnameTemplate, frameNumber, imageBuffer, bufferWidth, bufferHeight, bufferPitch);
		break;

	case CFHD_PIXEL_FORMAT_R210:
		WriteR210(pathnameTemplate, frameNumber, imageBuffer, bufferWidth, bufferHeight, bufferPitch);
		break;

	case CFHD_PIXEL_FORMAT_AB10:
	case CFHD_PIXEL_FORMAT_RG30:
		WriteRG30(pathnameTemplate, frameNumber, imageBuffer, bufferWidth, bufferHeight, bufferPitch);
		break;

	case CFHD_PIXEL_FORMAT_AR10:
		WriteAR10(pathnameTemplate, frameNumber, imageBuffer, bufferWidth, bufferHeight, bufferPitch);
		break;

	case CFHD_PIXEL_FORMAT_BYR2:
	case CFHD_PIXEL_FORMAT_BYR4:
		WriteBayer(pathnameTemplate, frameNumber, imageBuffer, bufferWidth, bufferHeight, bufferPitch,
				   bayerFormat);
		break;

	case CFHD_PIXEL_FORMAT_V210:
		WriteV210(pathnameTemplate, frameNumber, imageBuffer, bufferWidth, bufferHeight, bufferPitch);
		break;

	case CFHD_PIXEL_FORMAT_2VUY:
		Write2VUY(pathnameTemplate, frameNumber, imageBuffer, bufferWidth, bufferHeight, bufferPitch);
		break;

	case CFHD_PIXEL_FORMAT_YUY2:
		WriteYUY2(pathnameTemplate, frameNumber, imageBuffer, bufferWidth, bufferHeight, bufferPitch);
		break;

	case CFHD_PIXEL_FORMAT_YU64:
		WriteYU64(pathnameTemplate, frameNumber, imageBuffer, bufferWidth, bufferHeight, bufferPitch);
		break;

	default:
		assert(0);
		break;
	}
}

void WriteBayer(char *pathnameTemplate,		// Format string with a decimal format specifier
				int frameNumber,			// Frame number
				uint16_t *imageBuffer,		// Buffer of Bayer pixel data
				int bufferWidth,			// Width of buffer in values (not Bayer quads)
				int bufferHeight,			// Height of buffer in values (not Bayer quads)
				int bufferPitch,			// Number of bytes per row
				int bayerFormat)			// Bayer format
{
	// Bayer output is half size if a demosaic algorithm is not used
	int outputWidth = bufferWidth / 2;
	int outputHeight = bufferHeight / 2;

	// Allocate a buffer for one row of packed DPX pixels (10-bit RGB)
	const int pixelSize = sizeof(uint32_t);
	size_t outputRowSize = outputWidth * pixelSize;
	CRowBuffer outputBuffer(outputRowSize);
	if (outputBuffer == NULL) return;

	// Initialize a DPX file object
	CDPXFile file(outputWidth, outputHeight);

	char outputPathname[PATH_MAX];
	sprintf(outputPathname, pathnameTemplate, frameNumber);

	// Open a DPX file and write the file headers
	file.Create(outputPathname);
	fprintf(stderr, "%s\n", outputPathname);

	char *rowptr1 = (char *)imageBuffer;
	for (int row = 0; row < bufferHeight; row += 2)
	{
		char *rowptr2 = rowptr1 + bufferPitch;

		uint16_t *input1 = (uint16_t *)rowptr1;
		uint16_t *input2 = (uint16_t *)rowptr2;

		uint32_t *outptr = outputBuffer;

		for (int column = 0; column < bufferWidth; column += 2)
		{
			int r1;
			int g1;
			int g2;
			int b1;

			switch (bayerFormat)
			{
			case CFHD_BAYER_FORMAT_RED_GRN:
				r1 = input1[column + 0];
				g1 = input1[column + 1];
				g2 = input2[column + 0];
				b1 = input2[column + 1];
				break;

			case CFHD_BAYER_FORMAT_GRN_RED:
				g1 = input1[column + 0];
				r1 = input1[column + 1];
				b1 = input2[column + 0];
				g2 = input2[column + 1];
				break;

			default:
				//TODO: Unpack the pixels for other Bayer formats
				assert(0);
				break;
			}

			// Combine the green values
			g1 = (g1 + g2) / 2;

			// Pack the values into the row buffer
			*(outptr++) = Pack10(r1, g1, b1);
		}

		// Write one row of DPX pixels
		file.WriteRow(outputBuffer, outputRowSize);

		// Advance the row pointer by two rows
		rowptr1 = rowptr2 + bufferPitch;
	}
}

void WriteARGB64(char *pathnameTemplate,	// Format string with a decimal format specifier
				 int frameNumber,			// Frame number
				 uint16_t *imageBuffer,		// Buffer of pixel data in b64a format
				 int bufferWidth,			// Width of buffer in pixels
				 int bufferHeight,			// Height of buffer
				 int bufferPitch)			// Number of bytes per row
{
	// Output the buffer at full resolution
	int outputWidth = bufferWidth;
	int outputHeight = bufferHeight;

	// Allocate a buffer for one row of packed DPX pixels (10-bit RGB)
	const int pixelSize = sizeof(uint32_t);
	size_t outputRowSize = outputWidth * pixelSize;
	CRowBuffer outputBuffer(outputRowSize);
	if (outputBuffer == NULL) return;

	// Initialize a DPX file object
	CDPXFile file(outputWidth, outputHeight);

	char outputPathname[PATH_MAX];
	sprintf(outputPathname, pathnameTemplate, frameNumber);

	// Open a DPX file and write the file headers
	file.Create(outputPathname);

	fprintf(stderr, "%s\n", outputPathname);

	char *rowptr = (char *)imageBuffer;
	for (int row = 0; row < bufferHeight; row++)
	{
		uint16_t *input = (uint16_t *)rowptr;
		uint32_t *outptr = outputBuffer;

		for (int column = 0; column < bufferWidth; column++)
		{
			int a = input[4 * column + 0];
			int r = input[4 * column + 1];
			int g = input[4 * column + 2];
			int b = input[4 * column + 3];
		
			int x,y;
			x = column / 32;
			y = row / 32;
			if((x+y) & 1)
			{
				r = (0x8000 * (256 - (a>>8)) +  r * (a>>8)) >> 8;
				g = (0x8000 * (256 - (a>>8)) +  g * (a>>8)) >> 8;
				b = (0x8000 * (256 - (a>>8)) +  b * (a>>8)) >> 8;
			}
			else
			{
				r = (0x5000 * (256 - (a>>8)) +  r * (a>>8)) >> 8;
				g = (0x5000 * (256 - (a>>8)) +  g * (a>>8)) >> 8;
				b = (0x5000 * (256 - (a>>8)) +  b * (a>>8)) >> 8;
			}

			// Pack the values into the row buffer
			*(outptr++) = Pack10(r, g, b);
		}

		// Write one row of DPX pixels
		file.WriteRow(outputBuffer, outputRowSize);

		// Advance the row pointer by two rows
		rowptr += bufferPitch;
	}
}

void WriteRG48(char *pathnameTemplate,	// Format string with a decimal format specifier
				 int frameNumber,			// Frame number
				 uint16_t *imageBuffer,		// Buffer of pixel data in b64a format
				 int bufferWidth,			// Width of buffer in pixels
				 int bufferHeight,			// Height of buffer
				 int bufferPitch)			// Number of bytes per row
{
	// Output the buffer at full resolution
	int outputWidth = bufferWidth;
	int outputHeight = bufferHeight;

	// Allocate a buffer for one row of packed DPX pixels (10-bit RGB)
	const int pixelSize = sizeof(uint32_t);
	size_t outputRowSize = outputWidth * pixelSize;
	CRowBuffer outputBuffer(outputRowSize);
	if (outputBuffer == NULL) return;

	// Initialize a DPX file object
	CDPXFile file(outputWidth, outputHeight);

	char outputPathname[PATH_MAX];
	sprintf(outputPathname, pathnameTemplate, frameNumber);

	// Open a DPX file and write the file headers
	file.Create(outputPathname);

	fprintf(stderr, "%s\n", outputPathname);

	char *rowptr = (char *)imageBuffer;
	for (int row = 0; row < bufferHeight; row++)
	{
		uint16_t *input = (uint16_t *)rowptr;
		uint32_t *outptr = outputBuffer;

		for (int column = 0; column < bufferWidth; column++)
		{
			int r = input[3 * column + 0];
			int g = input[3 * column + 1];
			int b = input[3 * column + 2];

			// Pack the values into the row buffer
			*(outptr++) = Pack10(r, g, b);
		}

		// Write one row of DPX pixels
		file.WriteRow(outputBuffer, outputRowSize);

		// Advance the row pointer by two rows
		rowptr += bufferPitch;
	}
}

void WriteWP13(char *pathnameTemplate,	// Format string with a decimal format specifier
				 int frameNumber,			// Frame number
				 uint16_t *imageBuffer,		// Buffer of pixel data in b64a format
				 int bufferWidth,			// Width of buffer in pixels
				 int bufferHeight,			// Height of buffer
				 int bufferPitch)			// Number of bytes per row
{
	// Output the buffer at full resolution
	int outputWidth = bufferWidth;
	int outputHeight = bufferHeight;

	// Allocate a buffer for one row of packed DPX pixels (10-bit RGB)
	const int pixelSize = sizeof(uint32_t);
	size_t outputRowSize = outputWidth * pixelSize;
	CRowBuffer outputBuffer(outputRowSize);
	if (outputBuffer == NULL) return;

	// Initialize a DPX file object
	CDPXFile file(outputWidth, outputHeight);

	char outputPathname[PATH_MAX];
	sprintf(outputPathname, pathnameTemplate, frameNumber);

	// Open a DPX file and write the file headers
	file.Create(outputPathname);

	fprintf(stderr, "%s\n", outputPathname);

	char *rowptr = (char *)imageBuffer;
	for (int row = 0; row < bufferHeight; row++)
	{
		short *input = (short *)rowptr;
		uint32_t *outptr = outputBuffer;

		for (int column = 0; column < bufferWidth; column++)
		{
			int r = (input[3 * column + 0]<<3);
			int g = (input[3 * column + 1]<<3);
			int b = (input[3 * column + 2]<<3);

			if(r < 0) r = 0;
			if(g < 0) g = 0;
			if(b < 0) b = 0;
			if(r > 65535) r = 65535;
			if(g > 65535) g = 65535;
			if(b > 65535) b = 65535;

			// Pack the values into the row buffer
			*(outptr++) = Pack10(r, g, b);
		}

		// Write one row of DPX pixels
		file.WriteRow(outputBuffer, outputRowSize);

		// Advance the row pointer by two rows
		rowptr += bufferPitch;
	}
}


void WriteW13A(char *pathnameTemplate,	// Format string with a decimal format specifier
				 int frameNumber,			// Frame number
				 uint16_t *imageBuffer,		// Buffer of pixel data in b64a format
				 int bufferWidth,			// Width of buffer in pixels
				 int bufferHeight,			// Height of buffer
				 int bufferPitch)			// Number of bytes per row
{
	// Output the buffer at full resolution
	int outputWidth = bufferWidth;
	int outputHeight = bufferHeight;

	// Allocate a buffer for one row of packed DPX pixels (10-bit RGB)
	const int pixelSize = sizeof(uint32_t);
	size_t outputRowSize = outputWidth * pixelSize;
	CRowBuffer outputBuffer(outputRowSize);
	if (outputBuffer == NULL) return;

	// Initialize a DPX file object
	CDPXFile file(outputWidth, outputHeight);

	char outputPathname[PATH_MAX];
	sprintf(outputPathname, pathnameTemplate, frameNumber);

	// Open a DPX file and write the file headers
	file.Create(outputPathname);

	fprintf(stderr, "%s\n", outputPathname);

	char *rowptr = (char *)imageBuffer;
	for (int row = 0; row < bufferHeight; row++)
	{
		short *input = (short *)rowptr;
		uint32_t *outptr = outputBuffer;

		for (int column = 0; column < bufferWidth; column++)
		{
			int r = (input[4 * column + 0]<<3);
			int g = (input[4 * column + 1]<<3);
			int b = (input[4 * column + 2]<<3);
			int a = (input[4 * column + 3]<<3);

			if(r < 0) r = 0;
			if(g < 0) g = 0;
			if(b < 0) b = 0;
			if(r > 65535) r = 65535;
			if(g > 65535) g = 65535;
			if(b > 65535) b = 65535;

			int x,y;
			x = column / 32;
			y = row / 32;
			if((x+y) & 1)
			{
				r = (0x8000 * (256 - (a>>8)) +  r * (a>>8)) >> 8;
				g = (0x8000 * (256 - (a>>8)) +  g * (a>>8)) >> 8;
				b = (0x8000 * (256 - (a>>8)) +  b * (a>>8)) >> 8;
			}
			else
			{
				r = (0x5000 * (256 - (a>>8)) +  r * (a>>8)) >> 8;
				g = (0x5000 * (256 - (a>>8)) +  g * (a>>8)) >> 8;
				b = (0x5000 * (256 - (a>>8)) +  b * (a>>8)) >> 8;
			}

			// Pack the values into the row buffer
			*(outptr++) = Pack10(r, g, b);
		}

		// Write one row of DPX pixels
		file.WriteRow(outputBuffer, outputRowSize);

		// Advance the row pointer by two rows
		rowptr += bufferPitch;
	}
}


void WriteV210(char *pathnameTemplate,	// Format string with a decimal format specifier
				 int frameNumber,			// Frame number
				 uint16_t *imageBuffer,		// Buffer of pixel data in b64a format
				 int bufferWidth,			// Width of buffer in pixels
				 int bufferHeight,			// Height of buffer
				 int bufferPitch)			// Number of bytes per row
{
	// Output the buffer at full resolution
	int outputWidth = bufferWidth;
	int outputHeight = bufferHeight;

	// Allocate a buffer for one row of packed DPX pixels (10-bit RGB)
	const int pixelSize = sizeof(uint32_t);
	size_t outputRowSize = outputWidth * pixelSize;
	CRowBuffer outputBuffer(outputRowSize);
	if (outputBuffer == NULL) return;

	// Initialize a DPX file object
	CDPXFile file(outputWidth, outputHeight);

	char outputPathname[PATH_MAX];
	sprintf(outputPathname, pathnameTemplate, frameNumber);

	// Open a DPX file and write the file headers
	file.Create(outputPathname);

	fprintf(stderr, "%s\n", outputPathname);

	char *rowptr = (char *)imageBuffer;
	for (int row = 0; row < bufferHeight; row++)
	{
		uint32_t *input = (uint32_t *)rowptr;
		uint32_t *outptr = outputBuffer;
		int pos = 0;

		for (int column = 0; column < bufferWidth; column+=2)
		{
			int u,y1,v,y2;
			int R, G, B;
			uint32_t *lptr = (uint32_t *)input;
			lptr += (column/6)*4;

			switch(column % 6)
			{
			case 0:
				u = ((*lptr>>02) & 0xff) - 128; 
				y1= ((*lptr>>12) & 0xff); 
				v = ((*lptr>>22) & 0xff) - 128; 
				lptr++;
				y2= ((*lptr>>02) & 0xff);
				break;
			case 2:
				lptr++;
				y1= ((*lptr>>22) & 0xff); 
				lptr++;
				v = ((*lptr>>02) & 0xff) - 128; 
				y2= ((*lptr>>12) & 0xff); 
				u = ((*lptr>>22) & 0xff) - 128; 
				break;
			case 4:
				lptr+=2;
				u = ((*lptr>>22) & 0xff) - 128; 
				lptr++;
				y1= ((*lptr>>02) & 0xff); 
				v = ((*lptr>>12) & 0xff) - 128; 
				y2= ((*lptr>>22) & 0xff);
				break;
			}


			if(CGRGB)//cgrgb 
			{
				y1-=16;
				y2-=16;

				R = (9535*y1 + 14688*v)>>5;
				G = (9535*y1 - 4375*v - 1745*u)>>5;
				B = (9535*y1 + 17326*u)>>5;
			}
			else
			{
				R = (8192*y1 + 12616*v)>>5;
				G = (8192*y1 - 3760*v - 1499*u)>>5;
				B = (8192*y1 + 14877*u)>>5;
			}

			if(R<0) R=0; if(R>65535) R=65535;
			if(G<0) G=0; if(G>65535) G=65535;
			if(B<0) B=0; if(B>65535) B=65535;

			// Pack the values into the row buffer
			*(outptr++) = Pack10(R, G, B);
			
			if(CGRGB)//cgrgb 
			{				
				R = (9535*y2 + 14688*v)>>5;
				G = (9535*y2 - 4375*v - 1745*u)>>5;
				B = (9535*y2 + 17326*u)>>5;
			}
			else
			{
				R = (8192*y2 + 12616*v)>>5;
				G = (8192*y2 - 3760*v - 1499*u)>>5;
				B = (8192*y2 + 14877*u)>>5;
			}

			if(R<0) R=0; if(R>65535) R=65535;
			if(G<0) G=0; if(G>65535) G=65535;
			if(B<0) B=0; if(B>65535) B=65535;

			*(outptr++) = Pack10(R, G, B);
		}

		// Write one row of DPX pixels
		file.WriteRow(outputBuffer, outputRowSize);

		// Advance the row pointer by two rows
		rowptr += bufferPitch;
	}
}



void Write2VUY(char *pathnameTemplate,	// Format string with a decimal format specifier
				 int frameNumber,			// Frame number
				 uint16_t *imageBuffer,		// Buffer of pixel data in b64a format
				 int bufferWidth,			// Width of buffer in pixels
				 int bufferHeight,			// Height of buffer
				 int bufferPitch)			// Number of bytes per row
{
	// Output the buffer at full resolution
	int outputWidth = bufferWidth;
	int outputHeight = bufferHeight;

	// Allocate a buffer for one row of packed DPX pixels (10-bit RGB)
	const int pixelSize = sizeof(uint32_t);
	size_t outputRowSize = outputWidth * pixelSize;
	CRowBuffer outputBuffer(outputRowSize);
	if (outputBuffer == NULL) return;

	// Initialize a DPX file object
	CDPXFile file(outputWidth, outputHeight);

	char outputPathname[PATH_MAX];
	sprintf(outputPathname, pathnameTemplate, frameNumber);

	// Open a DPX file and write the file headers
	file.Create(outputPathname);

	fprintf(stderr, "%s\n", outputPathname);

	char *rowptr = (char *)imageBuffer;
	for (int row = 0; row < bufferHeight; row++)
	{
		uint16_t *input = (uint16_t *)rowptr;
		uint32_t *outptr = outputBuffer;
		int pos = 0;

		for (int column = 0; column < bufferWidth; column+=2)
		{
			int u = (input[column] & 0xff) - 128;
			int y1 = ((input[column] >> 8) & 0xff);
			int v = (input[column+1] & 0xff) - 128;
			int y2 = ((input[column+1] >> 8) & 0xff);
			int R, G, B;

			if(CGRGB)//cgrgb 
			{
				y1-=16;
				y2-=16;

				R = (9535*y1 + 14688*v)>>5;
				G = (9535*y1 - 4375*v - 1745*u)>>5;
				B = (9535*y1 + 17326*u)>>5;
			}
			else
			{
				R = (8192*y1 + 12616*v)>>5;
				G = (8192*y1 - 3760*v - 1499*u)>>5;
				B = (8192*y1 + 14877*u)>>5;
			}

			if(R<0) R=0; if(R>65535) R=65535;
			if(G<0) G=0; if(G>65535) G=65535;
			if(B<0) B=0; if(B>65535) B=65535;

			// Pack the values into the row buffer
			*(outptr++) = Pack10(R, G, B);
			
			if(CGRGB)//cgrgb 
			{				
				R = (9535*y2 + 14688*v)>>5;
				G = (9535*y2 - 4375*v - 1745*u)>>5;
				B = (9535*y2 + 17326*u)>>5;
			}
			else
			{
				R = (8192*y2 + 12616*v)>>5;
				G = (8192*y2 - 3760*v - 1499*u)>>5;
				B = (8192*y2 + 14877*u)>>5;
			}

			if(R<0) R=0; if(R>65535) R=65535;
			if(G<0) G=0; if(G>65535) G=65535;
			if(B<0) B=0; if(B>65535) B=65535;

			*(outptr++) = Pack10(R, G, B);
		}

		// Write one row of DPX pixels
		file.WriteRow(outputBuffer, outputRowSize);

		// Advance the row pointer by two rows
		rowptr += bufferPitch;
	}
}




void WriteYUY2(char *pathnameTemplate,	// Format string with a decimal format specifier
				 int frameNumber,			// Frame number
				 uint16_t *imageBuffer,		// Buffer of pixel data in b64a format
				 int bufferWidth,			// Width of buffer in pixels
				 int bufferHeight,			// Height of buffer
				 int bufferPitch)			// Number of bytes per row
{
	// Output the buffer at full resolution
	int outputWidth = bufferWidth;
	int outputHeight = bufferHeight;

	// Allocate a buffer for one row of packed DPX pixels (10-bit RGB)
	const int pixelSize = sizeof(uint32_t);
	size_t outputRowSize = outputWidth * pixelSize;
	CRowBuffer outputBuffer(outputRowSize);
	if (outputBuffer == NULL) return;

	// Initialize a DPX file object
	CDPXFile file(outputWidth, outputHeight);

	char outputPathname[PATH_MAX];
	sprintf(outputPathname, pathnameTemplate, frameNumber);

	// Open a DPX file and write the file headers
	file.Create(outputPathname);

	fprintf(stderr, "%s\n", outputPathname);

	char *rowptr = (char *)imageBuffer;
	for (int row = 0; row < bufferHeight; row++)
	{
		uint16_t *input = (uint16_t *)rowptr;
		uint32_t *outptr = outputBuffer;
		int pos = 0;

		for (int column = 0; column < bufferWidth; column+=2)
		{
			int y1 = (input[column] & 0xff);
			int u = ((input[column] >> 8) & 0xff) - 128;
			int y2 = (input[column+1] & 0xff);
			int v = ((input[column+1] >> 8) & 0xff) - 128;
			int R, G, B;

			if(CGRGB)//cgrgb 
			{
				y1-=16;
				y2-=16;

				R = (9535*y1 + 14688*v)>>5;
				G = (9535*y1 - 4375*v - 1745*u)>>5;
				B = (9535*y1 + 17326*u)>>5;
			}
			else
			{
				R = (8192*y1 + 12616*v)>>5;
				G = (8192*y1 - 3760*v - 1499*u)>>5;
				B = (8192*y1 + 14877*u)>>5;
			}

			if(R<0) R=0; if(R>65535) R=65535;
			if(G<0) G=0; if(G>65535) G=65535;
			if(B<0) B=0; if(B>65535) B=65535;

			// Pack the values into the row buffer
			*(outptr++) = Pack10(R, G, B);
			
			if(CGRGB)//cgrgb 
			{				
				R = (9535*y2 + 14688*v)>>5;
				G = (9535*y2 - 4375*v - 1745*u)>>5;
				B = (9535*y2 + 17326*u)>>5;
			}
			else
			{
				R = (8192*y2 + 12616*v)>>5;
				G = (8192*y2 - 3760*v - 1499*u)>>5;
				B = (8192*y2 + 14877*u)>>5;
			}

			if(R<0) R=0; if(R>65535) R=65535;
			if(G<0) G=0; if(G>65535) G=65535;
			if(B<0) B=0; if(B>65535) B=65535;

			*(outptr++) = Pack10(R, G, B);
		}

		// Write one row of DPX pixels
		file.WriteRow(outputBuffer, outputRowSize);

		// Advance the row pointer by two rows
		rowptr += bufferPitch;
	}
}


void WriteYU64(char *pathnameTemplate,	// Format string with a decimal format specifier
				 int frameNumber,			// Frame number
				 uint16_t *imageBuffer,		// Buffer of pixel data in b64a format
				 int bufferWidth,			// Width of buffer in pixels
				 int bufferHeight,			// Height of buffer
				 int bufferPitch)			// Number of bytes per row
{
	// Output the buffer at full resolution
	int outputWidth = bufferWidth;
	int outputHeight = bufferHeight;

	// Allocate a buffer for one row of packed DPX pixels (10-bit RGB)
	const int pixelSize = sizeof(uint32_t);
	size_t outputRowSize = outputWidth * pixelSize;
	CRowBuffer outputBuffer(outputRowSize);
	if (outputBuffer == NULL) return;

	// Initialize a DPX file object
	CDPXFile file(outputWidth, outputHeight);

	char outputPathname[PATH_MAX];
	sprintf(outputPathname, pathnameTemplate, frameNumber);

	// Open a DPX file and write the file headers
	file.Create(outputPathname);

	fprintf(stderr, "%s\n", outputPathname);

	char *rowptr = (char *)imageBuffer;
	for (int row = 0; row < bufferHeight; row++)
	{
		uint32_t *input = (uint32_t *)rowptr;
		uint32_t *outptr = outputBuffer;
		int pos = 0;

		for (int column = 0; column < bufferWidth; column+=2)
		{
			int y1 = (input[column] & 0xffff);
			int v = ((input[column] >> 16) & 0xffff) - (128<<8);
			int y2 = (input[column+1] & 0xffff);
			int u = ((input[column+1] >> 16) & 0xffff) - (128<<8);
			int R, G, B;

			if(CGRGB)//cgrgb 
			{
				y1-=(16<<8);
				y2-=(16<<8);

				R = (9535*y1 + 14688*v)>>13;
				G = (9535*y1 - 4375*v - 1745*u)>>13;
				B = (9535*y1 + 17326*u)>>13;
			}
			else
			{
				R = (8192*y1 + 12616*v)>>13;
				G = (8192*y1 - 3760*v - 1499*u)>>13;
				B = (8192*y1 + 14877*u)>>13;
			}

			if(R<0) R=0; if(R>65535) R=65535;
			if(G<0) G=0; if(G>65535) G=65535;
			if(B<0) B=0; if(B>65535) B=65535;

			// Pack the values into the row buffer
			*(outptr++) = Pack10(R, G, B);

			if(CGRGB)//cgrgb 
			{				
				R = (9535*y2 + 14688*v)>>13;
				G = (9535*y2 - 4375*v - 1745*u)>>13;
				B = (9535*y2 + 17326*u)>>13;
			}
			else
			{
				R = (8192*y2 + 12616*v)>>13;
				G = (8192*y2 - 3760*v - 1499*u)>>13;
				B = (8192*y2 + 14877*u)>>13;
			}

			if(R<0) R=0; if(R>65535) R=65535;
			if(G<0) G=0; if(G>65535) G=65535;
			if(B<0) B=0; if(B>65535) B=65535;

			*(outptr++) = Pack10(R, G, B);
		}

		// Write one row of DPX pixels
		file.WriteRow(outputBuffer, outputRowSize);

		// Advance the row pointer by two rows
		rowptr += bufferPitch;
	}
}





void WriteARGB32(char *pathnameTemplate,	// Format string with a decimal format specifier
				 int frameNumber,			// Frame number
				 uint16_t *imageBuffer,		// Buffer of pixel data in bgra format
				 int bufferWidth,			// Width of buffer in pixels
				 int bufferHeight,			// Height of buffer
				 int bufferPitch)			// Number of bytes per row
{
	// Output the buffer at full resolution
	int outputWidth = bufferWidth;
	int outputHeight = bufferHeight;

	// Allocate a buffer for one row of packed DPX pixels (10-bit RGB)
	const int pixelSize = sizeof(uint32_t);
	size_t outputRowSize = outputWidth * pixelSize;
	CRowBuffer outputBuffer(outputRowSize);
	if (outputBuffer == NULL) return;

	// Initialize a DPX file object
	CDPXFile file(outputWidth, outputHeight);

	char outputPathname[PATH_MAX];
	sprintf(outputPathname, pathnameTemplate, frameNumber);

	// Open a DPX file and write the file headers
	file.Create(outputPathname);

	fprintf(stderr, "%s\n", outputPathname);

	char *rowptr = (char *)imageBuffer;
	rowptr += (bufferHeight-1)*bufferPitch;
	for (int row = 0; row < bufferHeight; row++)
	{
		uint8_t  *input = (uint8_t  *)rowptr;
		uint32_t *outptr = outputBuffer;

		for (int column = 0; column < bufferWidth; column++)
		{
			int b = input[4 * column + 0]<<8;
			int g = input[4 * column + 1]<<8;
			int r = input[4 * column + 2]<<8;
			int a = input[4 * column + 3]<<8;

			
			int x,y;
			x = column / 32;
			y = row / 32;
			if((x+y) & 1)
			{
				r = (0x8000 * (256 - (a>>8)) +  r * (a>>8)) >> 8;
				g = (0x8000 * (256 - (a>>8)) +  g * (a>>8)) >> 8;
				b = (0x8000 * (256 - (a>>8)) +  b * (a>>8)) >> 8;
			}
			else
			{
				r = (0x5000 * (256 - (a>>8)) +  r * (a>>8)) >> 8;
				g = (0x5000 * (256 - (a>>8)) +  g * (a>>8)) >> 8;
				b = (0x5000 * (256 - (a>>8)) +  b * (a>>8)) >> 8;
			}

			// Pack the values into the row buffer
			*(outptr++) = Pack10(r, g, b);
		}

		// Write one row of DPX pixels
		file.WriteRow(outputBuffer, outputRowSize);

		// Advance the row pointer by two rows
		rowptr -= bufferPitch;
	}
}


void WriteRGB24(char *pathnameTemplate,	// Format string with a decimal format specifier
				 int frameNumber,			// Frame number
				 uint16_t *imageBuffer,		// Buffer of pixel data in bgra format
				 int bufferWidth,			// Width of buffer in pixels
				 int bufferHeight,			// Height of buffer
				 int bufferPitch)			// Number of bytes per row
{
	// Output the buffer at full resolution
	int outputWidth = bufferWidth;
	int outputHeight = bufferHeight;

	// Allocate a buffer for one row of packed DPX pixels (10-bit RGB)
	const int pixelSize = sizeof(uint32_t);
	size_t outputRowSize = outputWidth * pixelSize;
	CRowBuffer outputBuffer(outputRowSize);
	if (outputBuffer == NULL) return;

	// Initialize a DPX file object
	CDPXFile file(outputWidth, outputHeight);

	char outputPathname[PATH_MAX];
	sprintf(outputPathname, pathnameTemplate, frameNumber);

	// Open a DPX file and write the file headers
	file.Create(outputPathname);

	fprintf(stderr, "%s\n", outputPathname);

	char *rowptr = (char *)imageBuffer;
	rowptr += (bufferHeight-1)*bufferPitch;
	for (int row = 0; row < bufferHeight; row++)
	{
		uint8_t  *input = (uint8_t  *)rowptr;
		uint32_t *outptr = outputBuffer;

		for (int column = 0; column < bufferWidth; column++)
		{
			int b = input[3 * column + 0]<<8;
			int g = input[3 * column + 1]<<8;
			int r = input[3 * column + 2]<<8;
			
			// Pack the values into the row buffer
			*(outptr++) = Pack10(r, g, b);
		}

		// Write one row of DPX pixels
		file.WriteRow(outputBuffer, outputRowSize);

		// Advance the row pointer by two rows
		rowptr -= bufferPitch;
	}
}


void WriteR408(char *pathnameTemplate,	// Format string with a decimal format specifier
				 int frameNumber,			// Frame number
				 uint16_t *imageBuffer,		// Buffer of pixel data in bgra format
				 int bufferWidth,			// Width of buffer in pixels
				 int bufferHeight,			// Height of buffer
				 int bufferPitch)			// Number of bytes per row
{
	// Output the buffer at full resolution
	int outputWidth = bufferWidth;
	int outputHeight = bufferHeight;

	// Allocate a buffer for one row of packed DPX pixels (10-bit RGB)
	const int pixelSize = sizeof(uint32_t);
	size_t outputRowSize = outputWidth * pixelSize;
	CRowBuffer outputBuffer(outputRowSize);
	if (outputBuffer == NULL) return;

	// Initialize a DPX file object
	CDPXFile file(outputWidth, outputHeight);

	char outputPathname[PATH_MAX];
	sprintf(outputPathname, pathnameTemplate, frameNumber);

	// Open a DPX file and write the file headers
	file.Create(outputPathname);

	fprintf(stderr, "%s\n", outputPathname);

	char *rowptr = (char *)imageBuffer;
	for (int row = 0; row < bufferHeight; row++)
	{
		uint8_t  *input = (uint8_t  *)rowptr;
		uint32_t *outptr = outputBuffer;

		for (int column = 0; column < bufferWidth; column++)
		{
			int r,g,b;
			int a = input[4 * column + 0]<<8;
			int yy = input[4 * column + 1]<<8;
			int u = input[4 * column + 2]<<8;
			int v = input[4 * column + 3]<<8;

			r = g = b = yy; //hack

			
			int x,y;
			x = column / 32;
			y = row / 32;
			if((x+y) & 1)
			{
				r = (0x8000 * (256 - (a>>8)) +  r * (a>>8)) >> 8;
				g = (0x8000 * (256 - (a>>8)) +  g * (a>>8)) >> 8;
				b = (0x8000 * (256 - (a>>8)) +  b * (a>>8)) >> 8;
			}
			else
			{
				r = (0x5000 * (256 - (a>>8)) +  r * (a>>8)) >> 8;
				g = (0x5000 * (256 - (a>>8)) +  g * (a>>8)) >> 8;
				b = (0x5000 * (256 - (a>>8)) +  b * (a>>8)) >> 8;
			}

			// Pack the values into the row buffer
			*(outptr++) = Pack10(r, g, b);
		}

		// Write one row of DPX pixels
		file.WriteRow(outputBuffer, outputRowSize);

		// Advance the row pointer by two rows
		rowptr += bufferPitch;
	}
}


void WriteV408(char *pathnameTemplate,	// Format string with a decimal format specifier
				 int frameNumber,			// Frame number
				 uint16_t *imageBuffer,		// Buffer of pixel data in bgra format
				 int bufferWidth,			// Width of buffer in pixels
				 int bufferHeight,			// Height of buffer
				 int bufferPitch)			// Number of bytes per row
{
	// Output the buffer at full resolution
	int outputWidth = bufferWidth;
	int outputHeight = bufferHeight;

	// Allocate a buffer for one row of packed DPX pixels (10-bit RGB)
	const int pixelSize = sizeof(uint32_t);
	size_t outputRowSize = outputWidth * pixelSize;
	CRowBuffer outputBuffer(outputRowSize);
	if (outputBuffer == NULL) return;

	// Initialize a DPX file object
	CDPXFile file(outputWidth, outputHeight);

	char outputPathname[PATH_MAX];
	sprintf(outputPathname, pathnameTemplate, frameNumber);

	// Open a DPX file and write the file headers
	file.Create(outputPathname);

	fprintf(stderr, "%s\n", outputPathname);

	char *rowptr = (char *)imageBuffer;
	for (int row = 0; row < bufferHeight; row++)
	{
		uint8_t  *input = (uint8_t  *)rowptr;
		uint32_t *outptr = outputBuffer;

		for (int column = 0; column < bufferWidth; column++)
		{
			int r,g,b;
			int u = input[4 * column + 0]<<8;
			int yy = input[4 * column + 1]<<8;
			int v = input[4 * column + 2]<<8;
			int a = input[4 * column + 3]<<8;

			r = g = b = yy; //hack

			int x,y;
			x = column / 32;
			y = row / 32;
			if((x+y) & 1)
			{
				r = (0x8000 * (256 - (a>>8)) +  r * (a>>8)) >> 8;
				g = (0x8000 * (256 - (a>>8)) +  g * (a>>8)) >> 8;
				b = (0x8000 * (256 - (a>>8)) +  b * (a>>8)) >> 8;
			}
			else
			{
				r = (0x5000 * (256 - (a>>8)) +  r * (a>>8)) >> 8;
				g = (0x5000 * (256 - (a>>8)) +  g * (a>>8)) >> 8;
				b = (0x5000 * (256 - (a>>8)) +  b * (a>>8)) >> 8;
			}

			// Pack the values into the row buffer
			*(outptr++) = Pack10(r, g, b);
		}

		// Write one row of DPX pixels
		file.WriteRow(outputBuffer, outputRowSize);

		// Advance the row pointer by two rows
		rowptr += bufferPitch;
	}
}


void WriteR210(char *pathnameTemplate,	// Format string with a decimal format specifier
				 int frameNumber,			// Frame number
				 uint16_t *imageBuffer,		// Buffer of pixel data in r210 format
				 int bufferWidth,			// Width of buffer in pixels
				 int bufferHeight,			// Height of buffer
				 int bufferPitch)			// Number of bytes per row
{
	// Output the buffer at full resolution
	int outputWidth = bufferWidth;
	int outputHeight = bufferHeight;

	// Allocate a buffer for one row of packed DPX pixels (10-bit RGB)
	const int pixelSize = sizeof(uint32_t);
	size_t outputRowSize = outputWidth * pixelSize;
	CRowBuffer outputBuffer(outputRowSize);
	if (outputBuffer == NULL) return;

	// Initialize a DPX file object
	CDPXFile file(outputWidth, outputHeight);

	char outputPathname[PATH_MAX];
	sprintf(outputPathname, pathnameTemplate, frameNumber);

	// Open a DPX file and write the file headers
	file.Create(outputPathname);

	fprintf(stderr, "%s\n", outputPathname);

	char *rowptr = (char *)imageBuffer;
	for (int row = 0; row < bufferHeight; row++)
	{
		uint32_t *input = (uint32_t *)rowptr;
		uint32_t *outptr = outputBuffer;

		for (int column = 0; column < bufferWidth; column++)
		{
			// Pack the values into the row buffer
			*(outptr++) = Swap32(Swap32(input[column])<<2);
		}

		// Write one row of DPX pixels
		file.WriteRow(outputBuffer, outputRowSize);

		// Advance the row pointer by two rows
		rowptr += bufferPitch;
	}
}


void WriteDPX0(char *pathnameTemplate,	// Format string with a decimal format specifier
				 int frameNumber,			// Frame number
				 uint16_t *imageBuffer,		// Buffer of pixel data in r210 format
				 int bufferWidth,			// Width of buffer in pixels
				 int bufferHeight,			// Height of buffer
				 int bufferPitch)			// Number of bytes per row
{
	// Output the buffer at full resolution
	int outputWidth = bufferWidth;
	int outputHeight = bufferHeight;

	// Allocate a buffer for one row of packed DPX pixels (10-bit RGB)
	const int pixelSize = sizeof(uint32_t);
	size_t outputRowSize = outputWidth * pixelSize;
	CRowBuffer outputBuffer(outputRowSize);
	if (outputBuffer == NULL) return;

	// Initialize a DPX file object
	CDPXFile file(outputWidth, outputHeight);

	char outputPathname[PATH_MAX];
	sprintf(outputPathname, pathnameTemplate, frameNumber);

	// Open a DPX file and write the file headers
	file.Create(outputPathname);

	fprintf(stderr, "%s\n", outputPathname);

	char *rowptr = (char *)imageBuffer;
	for (int row = 0; row < bufferHeight; row++)
	{
		uint32_t *input = (uint32_t *)rowptr;
		uint32_t *outptr = outputBuffer;

		for (int column = 0; column < bufferWidth; column++)
		{
			// Pack the values into the row buffer
			*(outptr++) = input[column];
		}

		// Write one row of DPX pixels
		file.WriteRow(outputBuffer, outputRowSize);

		// Advance the row pointer by two rows
		rowptr += bufferPitch;
	}
}


void WriteRG30(char *pathnameTemplate,	// Format string with a decimal format specifier
				 int frameNumber,			// Frame number
				 uint16_t *imageBuffer,		// Buffer of pixel data in r210 format
				 int bufferWidth,			// Width of buffer in pixels
				 int bufferHeight,			// Height of buffer
				 int bufferPitch)			// Number of bytes per row
{
	// Output the buffer at full resolution
	int outputWidth = bufferWidth;
	int outputHeight = bufferHeight;

	// Allocate a buffer for one row of packed DPX pixels (10-bit RGB)
	const int pixelSize = sizeof(uint32_t);
	size_t outputRowSize = outputWidth * pixelSize;
	CRowBuffer outputBuffer(outputRowSize);
	if (outputBuffer == NULL) return;

	// Initialize a DPX file object
	CDPXFile file(outputWidth, outputHeight);

	char outputPathname[PATH_MAX];
	sprintf(outputPathname, pathnameTemplate, frameNumber);

	// Open a DPX file and write the file headers
	file.Create(outputPathname);

	fprintf(stderr, "%s\n", outputPathname);

	char *rowptr = (char *)imageBuffer;
	for (int row = 0; row < bufferHeight; row++)
	{
		uint32_t *input = (uint32_t *)rowptr;
		uint32_t *outptr = outputBuffer;

		for (int column = 0; column < bufferWidth; column++)
		{
			// Pack the values into the row buffer
			uint8_t a,b,c,d;
			int R,G,B;
			uint32_t val;

			val = input[column];

			R = (val>>20)&0x3ff;
			G = (val>>10)&0x3ff;
			B = val&0x3ff;

			val = (B<<20)|(G<<10)|R;
			val <<= 2;


			a = (uint8_t)(val>>24)&0xff;
			b = (uint8_t)(val>>16)&0xff;
			c = (uint8_t)(val>>8)&0xff;
			d = (uint8_t)(val>>0)&0xff;
			val = d<<24|c<<16|b<<8|a;

			*(outptr++) = val;
		}

		// Write one row of DPX pixels
		file.WriteRow(outputBuffer, outputRowSize);

		// Advance the row pointer by two rows
		rowptr += bufferPitch;
	}
}



void WriteAR10(char *pathnameTemplate,	// Format string with a decimal format specifier
				 int frameNumber,			// Frame number
				 uint16_t *imageBuffer,		// Buffer of pixel data in r210 format
				 int bufferWidth,			// Width of buffer in pixels
				 int bufferHeight,			// Height of buffer
				 int bufferPitch)			// Number of bytes per row
{
	// Output the buffer at full resolution
	int outputWidth = bufferWidth;
	int outputHeight = bufferHeight;

	// Allocate a buffer for one row of packed DPX pixels (10-bit RGB)
	const int pixelSize = sizeof(uint32_t);
	size_t outputRowSize = outputWidth * pixelSize;
	CRowBuffer outputBuffer(outputRowSize);
	if (outputBuffer == NULL) return;

	// Initialize a DPX file object
	CDPXFile file(outputWidth, outputHeight);

	char outputPathname[PATH_MAX];
	sprintf(outputPathname, pathnameTemplate, frameNumber);

	// Open a DPX file and write the file headers
	file.Create(outputPathname);

	fprintf(stderr, "%s\n", outputPathname);

	char *rowptr = (char *)imageBuffer;
	for (int row = 0; row < bufferHeight; row++)
	{
		uint32_t *input = (uint32_t *)rowptr;
		uint32_t *outptr = outputBuffer;

		for (int column = 0; column < bufferWidth; column++)
		{
			// Pack the values into the row buffer
			uint8_t a,b,c,d;
			int R,G,B;
			uint32_t val;

			val = input[column];

			B = (val>>20)&0x3ff;
			G = (val>>10)&0x3ff;
			R = val&0x3ff;

			val = (B<<20)|(G<<10)|R;
			val <<= 2;


			a = (uint8_t)((val>>24)&0xff);
			b = (uint8_t)((val>>16)&0xff);
			c = (uint8_t)((val>>8)&0xff);
			d = (uint8_t)((val>>0)&0xff);
			val = d<<24|c<<16|b<<8|a;

			*(outptr++) = val;
		}

		// Write one row of DPX pixels
		file.WriteRow(outputBuffer, outputRowSize);

		// Advance the row pointer by two rows
		rowptr += bufferPitch;
	}
}
