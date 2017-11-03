// $Header$

/*! @file DPXReader.cpp
*
*  @brief Routines for reading the DPX image files.
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
#include "CFHDEncoder.h"
#include "DPXReader.h"

#define Swap16(x)	(swap ? _byteswap_ushort(x) : x)
#define Swap32(x)	(swap ? _byteswap_ulong(x) : x)
#define Swap32f(x)	(swap ? (float)_byteswap_ulong((unsigned long)x) : x)

//TODO: Eliminate use of unsafe functions
#pragma warning(disable: 4996)


// Define a smart buffer for reading DPX pixels
class CDPXBuffer
{
public:

	CDPXBuffer(size_t size) :
		m_buffer(NULL)
	{
		m_buffer = malloc(size);
	}

	~CDPXBuffer()
	{
		if (m_buffer) {
			free(m_buffer);
			m_buffer = NULL;
		}
	}

	// DPX pixels are always packed into an integer number of double words
	operator uint32_t * () {
		return (uint32_t *)m_buffer;
	}

private:

	void *m_buffer;

};


// Define a simple DPX file reader
class CDPXFileReader
{
public:

	// Cineon DPX file format: http://www.cineon.com/ff_draft.php

	// Data types used in the Cineon DPX file format specification
	typedef unsigned char U8;
	typedef unsigned short U16;
	typedef unsigned long U32;
	typedef long S32;
	typedef float R32;
	typedef char ASCII;

	static const U32 SPDX = 0x53445058;
	static const U32 XPDS = 0x58504453;

	enum PixelFormat
	{
		kPixelFormat_RGB = 50,
	};

	CDPXFileReader() :
		m_file(NULL)
	{
	}

	CDPXFileReader(const char *pathname) :
		m_file(NULL)
	{
		Open(pathname);
	}

	~CDPXFileReader()
	{
		if (m_file != NULL) {
			fclose(m_file);
			m_file = NULL;
		}
	}

	// Open the DPX file and read the file headers
	bool Open(const char *filename);

	// Return true if the file has been opened
	bool IsOpen()
	{
		return (m_file != NULL);
	}

	// Read and unpack the entire frame
	bool ReadFrame(void *frameBuffer, int framePitch, CFHD_PixelFormat pixelFormat);

	// Return the DPX frame dimensions
	size_t FrameWidth()
	{
		return m_imageWidth;
	}

	size_t FrameHeight()
	{
		return m_imageHeight;
	}

	// Close the DPX file (return true if the file was open)
	bool Close();

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
		ASCII file_name[100];   /* image file name */
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
	
	size_t RoundUpBytes(size_t byteCount)
	{
		// DPX sizes are rounded to a 32-bit word
		return ((byteCount + 3) & ~((size_t)0x03));
	}

	// Unpack the 10-bit color components in a DPX pixel
	void Unpack10(uint32_t word, int *red, int *green, int *blue)
	{
		// Scale each color value to 16 bits
		const int shift = 6;

		const int red10 = 22;
		const int green10 = 12;
		const int blue10 = 2;

		const uint32_t mask10 = 0x3FF;

		int swap = 1;
		// Swap the input pixel (if necessary)
		word = Swap32(word);

		// Shift and mask the DPX pixel to extract the components
		*red = ((word >> red10) & mask10) << shift;
		*green = ((word >> green10) & mask10) << shift;
		*blue = ((word >> blue10) & mask10) << shift;
	}

	// Read one row of DPX pixels
	bool ReadRow(uint32_t *rowPtr, size_t rowSize, size_t *actualSizeOut);

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

bool CDPXFileReader::Open(const char *pathname)
{
	size_t readCount;
	
	// Try to open the DPX file in binary mode (read only)
	m_file = fopen(pathname, "rb");
	if (!m_file) {
		return false;
	}

	// Use unbuffered reads
	setbuf(m_file, NULL);

	// Compute byte offsets to the headers (for debugging)
	size_t imageInfoOffset = sizeof(m_fileInfo);
	size_t imageHeaderOffset = imageInfoOffset + sizeof(m_imageHeader);
	#pragma unused(imageHeaderOffset)

	// Read the file information block
	readCount = fread(&m_fileInfo, sizeof(m_fileInfo), 1, m_file);
	if (readCount != 1) {
		Close();
		return false;
	}

	unsigned long swap = 0;

	// Check the magic number in the file information header
	if(m_fileInfo.magic_num == XPDS)
		swap = 1;

	// Compute the offset to the image data
	m_imageOffset = Swap32(m_fileInfo.offset);

	// Compute the size of all of the headers
	size_t headerSize = m_imageOffset;

	// Compute the size of the image data that follows the headers
	size_t fileSize = Swap32(m_fileInfo.file_size);
	size_t dataSize = fileSize - headerSize;

	// Get the ditto key
	m_dittoKey = Swap32(m_fileInfo.ditto_key);

	// Reduce the header size by the size of the file information block
	headerSize -= sizeof(m_fileInfo);

	if (headerSize > sizeof(m_imageInfo))
	{
		// Read the image information block
		readCount = fread(&m_imageInfo, sizeof(m_imageInfo), 1, m_file);
		if (readCount != 1) {
			Close();
			return false;
		}

		// Set the image dimensions
		m_imageWidth = Swap32(m_imageInfo.pixels_per_line);
		m_imageHeight = Swap32(m_imageInfo.lines_per_image_ele);

		// Get the low and high reference data
		m_refLowData = Swap32(m_imageInfo.image_element[0].ref_low_data);
        m_refLowQuantity = Swap32f(m_imageInfo.image_element[0].ref_low_quantity);
		m_refHighData = Swap32(m_imageInfo.image_element[0].ref_high_data);
		m_refHighQuantity = Swap32f(m_imageInfo.image_element[0].ref_high_quantity);

		// Set the pixel format information
		m_bitsPerPixel = m_imageInfo.image_element[0].bit_size;
		m_pixelPacking = Swap16(m_imageInfo.image_element[0].packing);
		m_pixelEncoding = Swap16(m_imageInfo.image_element[0].encoding);
		m_pixelFormat = m_imageInfo.image_element[0].descriptor;

		// Get the data offset to the image element
		m_dataOffset = Swap32(m_imageInfo.image_element[0].data_offset);

		// Get the padding at the end of each row of pixels
		size_t rowPadding = Swap32(m_imageInfo.image_element[0].eol_padding);
		#pragma unused (rowPadding)

		// Reduce the image data size by the end of image padding
		size_t imagePadding = Swap32(m_imageInfo.image_element[0].eo_image_padding);
		dataSize -= imagePadding;

		// Compute the number of bytes per image row
		m_bytesPerRow = dataSize / m_imageHeight;

		// Compute the number of bytes per image pixel
		m_pixelSize = m_bytesPerRow / m_imageWidth;

		// Reduce the header size by the size of the image information block
		headerSize -= sizeof(m_imageInfo);
	}
	else
	{
		//TODO: Indicate that the header could not be read (or return error)
		goto setFilePtr;
	}

	if (headerSize > sizeof(m_imageHeader))
	{
		// Read the image orientation block
		readCount = fread(&m_imageHeader, sizeof(m_imageHeader), 1, m_file);
		if (readCount != 1) {
			Close();
			return false;
		}

		// Set the pixel aspect ratio
		m_pixelAspectRatio.horizontal = Swap32(m_imageHeader.pixel_aspect[0]);
		m_pixelAspectRatio.vertical = Swap32(m_imageHeader.pixel_aspect[1]);

		// Reduce the header size by the size of the image orientation block
		headerSize -= sizeof(m_imageHeader);
	}
	else
	{
		//TODO: Indicate that the header could not be read (or return error)
		goto setFilePtr;
	}

	// Determine the type of industry specific headers that are present
	size_t industryHeaderSize = Swap32(m_fileInfo.ind_hdr_size);

	// Determine the size of the user defined data header
	size_t userDataHeaderSize = m_imageOffset - Swap32(m_fileInfo.gen_hdr_size) - industryHeaderSize;

	// Is the motion picture film header present?
	if (industryHeaderSize > sizeof(m_videoHeader))
	{
		// Read the motion picture film industry header
		readCount = fread(&m_filmHeader, sizeof(m_filmHeader), 1, m_file);
		if (readCount != 1) {
			Close();
			return false;
		}

		// Reduce the industry specific headers by the size of the motion picture film header
		industryHeaderSize -= sizeof(m_filmHeader);

		// Reduce the header size by the size of the motion picture film header
		headerSize -= sizeof(m_filmHeader);
	}

	// Is the television industry header present?
	if (industryHeaderSize == sizeof(m_videoHeader))
	{
		// Read the television industry header
		readCount = fread(&m_videoHeader, sizeof(m_videoHeader), 1, m_file);
		if (readCount != 1) {
			Close();
			return false;
		}

		// Set the video frame rate and gamma
		m_videoFrameRate = Swap32f(m_videoHeader.frame_rate);
		m_videoGamma = Swap32f(m_videoHeader.gamma);

		// Reduce the industry specific headers by the size of the television industry header
		industryHeaderSize -= sizeof(m_videoHeader);

		// Reduce the header size by the size of the television industry header
		headerSize -= sizeof(m_videoHeader);
	}
	else
	{
		//TODO: Indicate that no industry specific headers were present
		goto setFilePtr;
	}

	// Should have read all of the industry specific headers that are present
	assert(industryHeaderSize == 0);

	// The only header that has not been read is the user defined data header
	assert(userDataHeaderSize == headerSize);

setFilePtr:

	// Position the file pointer at the start of the image data
	if (fseek(m_file, (long)m_imageOffset, SEEK_SET) != 0) {
		Close();
		return false;
	}

	return true;
}

// Read the next row of pixels from the DPX file
bool CDPXFileReader::ReadRow(uint32_t *bufferPtr, size_t bufferSize, size_t *actualSizeOut)
{
	if (actualSizeOut) {
		*actualSizeOut = 0;
	}

	// Reduce the read request to one row
	size_t readSize = ((m_bytesPerRow <= bufferSize) ? m_bytesPerRow : bufferSize);

	// Read the next row of data into the buffer
	size_t readCount = fread(bufferPtr, readSize, 1, m_file);
	if (readCount != 1) {
		return false;
	}

	if (actualSizeOut) {
		*actualSizeOut = readSize;
	}

	return true;
}

// Read and unpack the entire frame in a DPX file
bool CDPXFileReader::ReadFrame(void *frameBuffer, int frameRowPitch, CFHD_PixelFormat pixelFormat)
{
	// Allocate a buffer for reading rows from the DPX file
	CDPXBuffer rowBuffer(m_bytesPerRow);
	uint32_t *bufferPtr = rowBuffer;
	assert(bufferPtr != NULL);

	uint8_t *frameRowPtr = (uint8_t *)frameBuffer;

	if(pixelFormat == CFHD_PIXEL_FORMAT_BGRA) // upside-down buffers
	{
		frameRowPtr += (m_imageHeight-1)*frameRowPitch;
		frameRowPitch = -frameRowPitch;
	}

	for (size_t row = 0; row < m_imageHeight; row++)
	{
		// Read the next row from the DPX file
		size_t actualReadSize = 0;
		if (!ReadRow(bufferPtr, m_bytesPerRow, &actualReadSize)) {
			return false;
		}

		if (actualReadSize != m_bytesPerRow) {
			return false;
		}

		if(pixelFormat == CFHD_PIXEL_FORMAT_R210)
		{
			int swap = 1;
			// Unpack the pixels in the row
			uint32_t *pixelPtr = (uint32_t *)bufferPtr;
			uint32_t *framePtr = (uint32_t *)frameRowPtr;
			for (; actualReadSize > 0; actualReadSize -= sizeof(uint32_t))
			{
				uint32_t pixel = *(pixelPtr++);
				int red, green, blue;
				Unpack10(pixel, &red, &green, &blue);
				red>>=6;
				green>>=6;
				blue>>=6;
				
				*framePtr++ = Swap32((red<<20)|(green<<10)|(blue));
			}
		}
		if(pixelFormat == CFHD_PIXEL_FORMAT_DPX0)
		{
			memcpy(frameRowPtr, bufferPtr, m_bytesPerRow);
		}
		else if(pixelFormat == CFHD_PIXEL_FORMAT_BGRA)
		{
			// Unpack the pixels in the row
			uint32_t *pixelPtr = (uint32_t *)bufferPtr;
			uint8_t *framePtr = (uint8_t *)frameRowPtr;
			for (; actualReadSize > 0; actualReadSize -= sizeof(uint32_t))
			{
				const int alpha = 255;

				// Unpack the next pixel
				uint32_t pixel = *(pixelPtr++);
				int red, green, blue;
				Unpack10(pixel, &red, &green, &blue);

				// Store the pixel components in the output buffer
				*(framePtr++) = blue>>8;
				*(framePtr++) = green>>8;
				*(framePtr++) = red>>8;
				*(framePtr++) = alpha;
			}

			// Advance to the next row in the output frame buffer
		}
		else if(pixelFormat == CFHD_PIXEL_FORMAT_2VUY || pixelFormat == CFHD_PIXEL_FORMAT_YUY2)
		{
			// Unpack the pixels in the row
			uint32_t *pixelPtr = (uint32_t *)bufferPtr;
			uint8_t *framePtr = (uint8_t *)frameRowPtr;
			for (; actualReadSize > 0; actualReadSize -= sizeof(uint32_t)*2)
			{
				const int alpha = 255;

				// Unpack the next pixel
				uint32_t pixel = *(pixelPtr++);
				int R, G, B,Y,Y2,U,V;
				Unpack10(pixel, &R, &G, &B);
				R >>= 6;
				G >>= 6;
				B >>= 6;

				Y =  (((1499 * R) + (5030 * G) + (508 * B))>>13) + 64;
				U =  ((((-827* R) - (2769 * G) + (3596* B))>>14));
				V =  ((((3596* R) - (3269 * G) - (328 * B))>>14));

				pixel = *(pixelPtr++);
				Unpack10(pixel, &R, &G, &B);
				R >>= 6;
				G >>= 6;
				B >>= 6;

				Y2 = (((1499 * R) + (5030 * G) + (508 * B))>>13) + 64;
				U += ((((-827* R) - (2769 * G) + (3596* B))>>14)) + 512;
				V += ((((3596* R) - (3269 * G) - (328 * B))>>14)) + 512;

				// Store the pixel components in the output buffer
				if(pixelFormat == CFHD_PIXEL_FORMAT_2VUY)
				{
					*(framePtr++) = U>>2;
					*(framePtr++) = Y>>2;
					*(framePtr++) = V>>2;
					*(framePtr++) = Y2>>2;
				}
				else
				{
					*(framePtr++) = Y>>2;
					*(framePtr++) = U>>2;
					*(framePtr++) = Y2>>2;
					*(framePtr++) = V>>2;
				}
			}

			// Advance to the next row in the output frame buffer
		}
		else if(pixelFormat == CFHD_PIXEL_FORMAT_V210)
		{
			// Unpack the pixels in the row
			uint32_t *pixelPtr = (uint32_t *)bufferPtr;
			uint32_t *framePtr = (uint32_t *)frameRowPtr;
			for (; actualReadSize > 0; actualReadSize -= sizeof(uint32_t)*6)
			{
				const int alpha = 255;

				// Unpack the next pixel
				uint32_t pixel;
				int R, G, B;
				int Y[6],U[3],V[3], yuv;

// Pixel parameters for the V210 format
#define V210_VALUE1_SHIFT		 0
#define V210_VALUE2_SHIFT		10
#define V210_VALUE3_SHIFT		20

#define V210_VALUE_MASK		0x03FF
		
				pixel = *(pixelPtr++);
				Unpack10(pixel, &R, &G, &B);
				R >>= 6;
				G >>= 6;
				B >>= 6;

				Y[0] =  (((1499 * R) + (5030 * G) + (508 * B))>>13) + 64;
				U[0] =  ((((-827* R) - (2769 * G) + (3596* B))>>14));
				V[0] =  ((((3596* R) - (3269 * G) - (328 * B))>>14));

				pixel = *(pixelPtr++);
				Unpack10(pixel, &R, &G, &B);
				R >>= 6;
				G >>= 6;
				B >>= 6;
				Y[1]  = (((1499 * R) + (5030 * G) + (508 * B))>>13) + 64;
				U[0] += ((((-827* R) - (2769 * G) + (3596* B))>>14)) + 512;
				V[0] += ((((3596* R) - (3269 * G) - (328 * B))>>14)) + 512;



				pixel = *(pixelPtr++);
				Unpack10(pixel, &R, &G, &B);
				R >>= 6;
				G >>= 6;
				B >>= 6;
				Y[2] =  (((1499 * R) + (5030 * G) + (508 * B))>>13) + 64;
				U[1] =  ((((-827* R) - (2769 * G) + (3596* B))>>14));
				V[1] =  ((((3596* R) - (3269 * G) - (328 * B))>>14));

				pixel = *(pixelPtr++);
				Unpack10(pixel, &R, &G, &B);
				R >>= 6;
				G >>= 6;
				B >>= 6;
				Y[3]  = (((1499 * R) + (5030 * G) + (508 * B))>>13) + 64;
				U[1] += ((((-827* R) - (2769 * G) + (3596* B))>>14)) + 512;
				V[1] += ((((3596* R) - (3269 * G) - (328 * B))>>14)) + 512;


				pixel = *(pixelPtr++);
				Unpack10(pixel, &R, &G, &B);
				R >>= 6;
				G >>= 6;
				B >>= 6;
				Y[4] =  (((1499 * R) + (5030 * G) + (508 * B))>>13) + 64;
				U[2] =  ((((-827* R) - (2769 * G) + (3596* B))>>14));
				V[2] =  ((((3596* R) - (3269 * G) - (328 * B))>>14));

				pixel = *(pixelPtr++);
				Unpack10(pixel, &R, &G, &B);
				R >>= 6;
				G >>= 6;
				B >>= 6;
				Y[5]  = (((1499 * R) + (5030 * G) + (508 * B))>>13) + 64;
				U[2] += ((((-827* R) - (2769 * G) + (3596* B))>>14)) + 512;
				V[2] += ((((3596* R) - (3269 * G) - (328 * B))>>14)) + 512;


				// Assemble and store the first word of packed values
				yuv = (V[0] << V210_VALUE3_SHIFT) | (Y[0] << V210_VALUE2_SHIFT) | (U[0] << V210_VALUE1_SHIFT);
				*(framePtr++) = yuv;

				// Assemble and store the second word of packed values
				yuv = (Y[2] << V210_VALUE3_SHIFT) | (U[1] << V210_VALUE2_SHIFT) | (Y[1] << V210_VALUE1_SHIFT);
				*(framePtr++) = yuv;

				// Assemble and store the third word of packed values
				yuv = (U[2] << V210_VALUE3_SHIFT) | (Y[3] << V210_VALUE2_SHIFT) | (V[1] << V210_VALUE1_SHIFT);
				*(framePtr++) = yuv;

				// Assemble and store the fourth word of packed values
				yuv = (Y[5] << V210_VALUE3_SHIFT) | (V[2] << V210_VALUE2_SHIFT) | (Y[4] << V210_VALUE1_SHIFT);
				*(framePtr++) = yuv;
			}

			// Advance to the next row in the output frame buffer
		}
		else if(pixelFormat == CFHD_PIXEL_FORMAT_YU64)
		{
			// Unpack the pixels in the row
			uint32_t *pixelPtr = (uint32_t *)bufferPtr;
			uint16_t *framePtr = (uint16_t *)frameRowPtr;
			for (; actualReadSize > 0; actualReadSize -= sizeof(uint32_t)*2)
			{
				const int alpha = 255;

				// Unpack the next pixel
				uint32_t pixel = *(pixelPtr++);
				int R, G, B,Y,Y2,U,V;
				Unpack10(pixel, &R, &G, &B);
				R >>= 6;
				G >>= 6;
				B >>= 6;
				
				Y =  (((1499 * R) + (5030 * G) + (508 * B))>>13) + 64;
				U =  ((((-827* R) - (2769 * G) + (3596* B))>>14));
				V =  ((((3596* R) - (3269 * G) - (328 * B))>>14));


				pixel = *(pixelPtr++);
				Unpack10(pixel, &R, &G, &B);
				R >>= 6;
				G >>= 6;
				B >>= 6;

				Y2=  (((1499 * R) + (5030 * G) + (508 * B))>>13) + 64;
				U += ((((-827* R) - (2769 * G) + (3596* B))>>14)) + 512;
				V += ((((3596* R) - (3269 * G) - (328 * B))>>14)) + 512;


				// Store the pixel components in the output buffer
				{
					*(framePtr++) = Y<<6;
					*(framePtr++) = V<<6;
					*(framePtr++) = Y2<<6;
					*(framePtr++) = U<<6;
				}
			}

			// Advance to the next row in the output frame buffer
		}
		else if(pixelFormat == CFHD_PIXEL_FORMAT_RG48)
		{
			// Unpack the pixels in the row
			uint32_t *pixelPtr = (uint32_t *)bufferPtr;
			uint16_t *framePtr = (uint16_t *)frameRowPtr;
			for (; actualReadSize > 0; actualReadSize -= sizeof(uint32_t))
			{
				const int alpha = USHRT_MAX;

				// Unpack the next pixel
				uint32_t pixel = *(pixelPtr++);
				int red, green, blue;
				Unpack10(pixel, &red, &green, &blue);

				// Store the pixel components in the output buffer
				*(framePtr++) = red;
				*(framePtr++) = green;
				*(framePtr++) = blue;
			}

			// Advance to the next row in the output frame buffer
		}
		else if(pixelFormat == CFHD_PIXEL_FORMAT_RG64)
		{
			assert(actualReadSize < 0xffffffff);
			uint32_t rsize = (uint32_t)actualReadSize;
			// Unpack the pixels in the row
			uint32_t *pixelPtr = (uint32_t *)bufferPtr;
			uint16_t *framePtr = (uint16_t *)frameRowPtr;
			for (; actualReadSize > 0; actualReadSize -= sizeof(uint32_t))
			{
				const int alpha = USHRT_MAX;

				// Unpack the next pixel
				uint32_t pixel = *(pixelPtr++);
				int red, green, blue;
				Unpack10(pixel, &red, &green, &blue);

				// Store the pixel components in the output buffer
				*(framePtr++) = red;
				*(framePtr++) = green;
				*(framePtr++) = blue;
				*(framePtr++) = (uint16_t)((float)alpha*(float)actualReadSize/(float)rsize);
			}

			// Advance to the next row in the output frame buffer
		}
		else if(pixelFormat == CFHD_PIXEL_FORMAT_B64A)
		{
			assert(actualReadSize < 0xffffffff);
			uint32_t rsize = (uint32_t)actualReadSize;
			// Unpack the pixels in the row
			uint32_t *pixelPtr = (uint32_t *)bufferPtr;
			uint16_t *framePtr = (uint16_t *)frameRowPtr;
			for (; actualReadSize > 0; actualReadSize -= sizeof(uint32_t))
			{
				const int alpha = USHRT_MAX;

				// Unpack the next pixel
				uint32_t pixel = *(pixelPtr++);
				int red, green, blue;
				Unpack10(pixel, &red, &green, &blue);

				// Store the pixel components in the output buffer
				*(framePtr++) = (uint16_t)((float)alpha*(float)actualReadSize/(float)rsize);
				*(framePtr++) = red;
				*(framePtr++) = green;
				*(framePtr++) = blue;
			}

			// Advance to the next row in the output frame buffer
		}
		else if(pixelFormat == CFHD_PIXEL_FORMAT_RG30 || pixelFormat == CFHD_PIXEL_FORMAT_AB10)
		{
			// Unpack the pixels in the row
			uint32_t *pixelPtr = (uint32_t *)bufferPtr;
			uint32_t *framePtr = (uint32_t *)frameRowPtr;
			for (; actualReadSize > 0; actualReadSize -= sizeof(uint32_t))
			{
				const int alpha = USHRT_MAX;

				// Unpack the next pixel
				uint32_t pixel = *(pixelPtr++);
				int red, green, blue;
				Unpack10(pixel, &red, &green, &blue);

				red >>= 6;
				green >>= 6;
				blue >>= 6;

				// Store the pixel components in the output buffer
				*framePtr++ = (blue<<20) | (green<<10) | red;
			}

			// Advance to the next row in the output frame buffer
		}
		else if(pixelFormat == CFHD_PIXEL_FORMAT_AR10)
		{
			// Unpack the pixels in the row
			uint32_t *pixelPtr = (uint32_t *)bufferPtr;
			uint32_t *framePtr = (uint32_t *)frameRowPtr;
			for (; actualReadSize > 0; actualReadSize -= sizeof(uint32_t))
			{
				const int alpha = USHRT_MAX;

				// Unpack the next pixel
				uint32_t pixel = *(pixelPtr++);
				int red, green, blue;
				Unpack10(pixel, &red, &green, &blue);

				red >>= 6;
				green >>= 6;
				blue >>= 6;

				// Store the pixel components in the output buffer
				*framePtr++ = (blue) | (green<<10) | (red<<20);
			}

			// Advance to the next row in the output frame buffer
		}
		else if(pixelFormat == CFHD_PIXEL_FORMAT_BYR4)
		{

			uint32_t *pixelPtr = (uint32_t *)bufferPtr;
			uint16_t *framePtr = (uint16_t *)frameRowPtr;
			for (; actualReadSize > 0; actualReadSize -= sizeof(uint32_t)*2)
			{
				// Unpack the next pixel
				uint32_t pixel;
				int red, green, blue;


				// Red-Grn top left Bayer phase
#if 1 //red-Grn
				pixel = *(pixelPtr++);
				Unpack10(pixel, &red, &green, &blue);
				if(row & 1)
					*framePtr++ = green;
				else
					*framePtr++ = red;
				
				pixel = *(pixelPtr++);
				Unpack10(pixel, &red, &green, &blue);
				if(row & 1)
					*framePtr++ = blue;
				else
					*framePtr++ = green;
#elif 0 //grn-blu
				pixel = *(pixelPtr++);
				Unpack10(pixel, &red, &green, &blue);
				if(row & 1)
					*framePtr++ = red;
				else
					*framePtr++ = green;

				pixel = *(pixelPtr++);
				Unpack10(pixel, &red, &green, &blue);
				if(row & 1)
					*framePtr++ = green;
				else
					*framePtr++ = blue;
#elif 0 //grn-red
				pixel = *(pixelPtr++);
				Unpack10(pixel, &red, &green, &blue);
				if(row & 1)
					*framePtr++ = blue;
				else
					*framePtr++ = green;

				pixel = *(pixelPtr++);
				Unpack10(pixel, &red, &green, &blue);
				if(row & 1)
					*framePtr++ = green;
				else
					*framePtr++ = red;
#elif  0 //blu-Grn
				pixel = *(pixelPtr++);
				Unpack10(pixel, &red, &green, &blue);
				if(row & 1)
					*framePtr++ = green;
				else
					*framePtr++ = blue;
				
				pixel = *(pixelPtr++);
				Unpack10(pixel, &red, &green, &blue);
				if(row & 1)
					*framePtr++ = red;
				else
					*framePtr++ = green;
#endif

					

				// Grn-Blu top left Bayer phase
			/*	if(row & 1)
					*framePtr++ = red;
				else
					*framePtr++ = green;

				
				pixel = *(pixelPtr++);
				Unpack10(pixel, &red, &green, &blue);

				if(row & 1)
					*framePtr++ = green;
				else
					*framePtr++ = blue;
					*/
			}
		}
		
		else if(pixelFormat == CFHD_PIXEL_FORMAT_BYR5) 
		{
			uint32_t *pixelPtr = (uint32_t *)bufferPtr;
			uint8_t *R8Ptr;
			uint8_t *G8Ptr;
			uint8_t *g8Ptr;
			uint8_t *B8Ptr;
			uint8_t *R4Ptr;
			uint8_t *G4Ptr;
			uint8_t *g4Ptr;
			uint8_t *B4Ptr;
			int width,x;

			width = frameRowPitch / 3;
			
			if((row & 1) == 0)
				R8Ptr = (uint8_t *)frameRowPtr;
			else
				R8Ptr = (uint8_t *)(frameRowPtr - frameRowPitch);

			G8Ptr = R8Ptr + width;
			g8Ptr = G8Ptr + width;
			B8Ptr = g8Ptr + width;
			R4Ptr = B8Ptr + width;
			G4Ptr = R4Ptr + (width>>1);
			g4Ptr = G4Ptr + (width>>1);
			B4Ptr = g4Ptr + (width>>1);

			if((row & 1) == 0)
			{
				for (x=0; x < width; x+=2)
				{
					// Unpack the next pixel
					uint32_t pixel;
					//int red, green, blue, rN, gN, bN;
					int red, green, blue, rN, gN;

					// Red-Grn top left Bayer phase
					pixel = *(pixelPtr++);
					Unpack10(pixel, &red, &green, &blue);
					red >>= 6; green >>= 6; blue >>= 6;
					*R8Ptr++ = red>>2;
					rN = (red<<2) & 0xf;

					pixel = *(pixelPtr++);
					Unpack10(pixel, &red, &green, &blue);
					red >>= 6; green >>= 6; blue >>= 6;
					*G8Ptr++ = green>>2;
					gN = (green<<2) & 0xf;


					pixel = *(pixelPtr++);
					Unpack10(pixel, &red, &green, &blue);
					red >>= 6; green >>= 6; blue >>= 6;
					*R8Ptr++ = red>>2;
					*R4Ptr++ = rN | ((red<<6) & 0xf0);

					pixel = *(pixelPtr++);
					Unpack10(pixel, &red, &green, &blue);
					red >>= 6; green >>= 6; blue >>= 6;
					*G8Ptr++ = green>>2;
					*G4Ptr++ = gN | ((green<<6) & 0xf0);
				}
			}
			else
			{
			// next line
				for (x=0; x < width; x+=2)
				{
					// Unpack the next pixel
					uint32_t pixel;
					//int red, green, blue, rN, gN, bN;
					int red, green, blue, gN, bN;

					pixel = *(pixelPtr++);
					Unpack10(pixel, &red, &green, &blue);
					red >>= 6; green >>= 6; blue >>= 6;
					*g8Ptr++ = green>>2;
					gN = (red<<2) & 0xf;

					pixel = *(pixelPtr++);
					Unpack10(pixel, &red, &green, &blue);
					red >>= 6; green >>= 6; blue >>= 6;
					*B8Ptr++ = blue>>2;
					bN = (blue<<2) & 0xf;

					pixel = *(pixelPtr++);
					Unpack10(pixel, &red, &green, &blue);
					red >>= 6; green >>= 6; blue >>= 6;
					*g8Ptr++ = green>>2;
					*g4Ptr++ = gN | ((green<<6) & 0xf0);
					red >>= 6; green >>= 6; blue >>= 6;

					pixel = *(pixelPtr++);
					Unpack10(pixel, &red, &green, &blue);
					red >>= 6; green >>= 6; blue >>= 6;
					*B8Ptr++ = blue>>2;
					*B4Ptr++ = bN | ((blue<<6) & 0xf0);
				}
			}
		}
		frameRowPtr += frameRowPitch;
	}

	// Successfully read and unpacked the entire frame from the DPX file
	return true;
}

bool CDPXFileReader::Close()
{
	if (m_file) {
		fclose(m_file);
		m_file = NULL;
		return true;
	}

	return false;
}


// Open a DPX file and read the header information
CFHD_Error DPXFileOpen(DPXFileReaderRef *fileRef,
					   const char *pathnameTemplate,
					   int frameNumber)
{
	//CFHD_Error error = CFHD_ERROR_OKAY;

	// Get the pathname for the DPX file corresponding to the frame number
	char pathname[_MAX_PATH];
	sprintf(pathname, pathnameTemplate, frameNumber);

	// Allocate a DPX file reader and read the DPX file headers
	CDPXFileReader *fileReader = new CDPXFileReader(pathname);
	if (fileReader == NULL) {
		return CFHD_ERROR_OUTOFMEMORY;
	}

	if (!fileReader->IsOpen()) {
		return CFHD_ERROR_BADFILE;
	}

	// Return the DPX file reader
	*fileRef = (DPXFileReaderRef)fileReader;

	return CFHD_ERROR_OKAY;
}

// Read a frame from the DPX file
CFHD_Error DPXReadFrame(DPXFileReaderRef fileRef,
						void *frameBuffer,
						int framePitch,
						CFHD_PixelFormat pixelFormat)
{
	CDPXFileReader *fileReader = (CDPXFileReader *)fileRef;
	assert(fileRef != NULL);
	if (! (fileRef != NULL)) {
		return CFHD_ERROR_INVALID_ARGUMENT;
	}

	if (!fileReader->IsOpen()) {
		return CFHD_ERROR_BADFILE;
	}

	assert(frameBuffer != NULL && framePitch > 0);

	switch (pixelFormat)
	{
	//TODO: Add more output formats
	case CFHD_PIXEL_FORMAT_BGRA:
	case CFHD_PIXEL_FORMAT_BYR4:
	case CFHD_PIXEL_FORMAT_BYR5:
	case CFHD_PIXEL_FORMAT_AB10:
	case CFHD_PIXEL_FORMAT_AR10:
	case CFHD_PIXEL_FORMAT_RG30:
	case CFHD_PIXEL_FORMAT_R210:
	case CFHD_PIXEL_FORMAT_DPX0:
	case CFHD_PIXEL_FORMAT_RG48:
	case CFHD_PIXEL_FORMAT_RG64:
	case CFHD_PIXEL_FORMAT_B64A:
	case CFHD_PIXEL_FORMAT_2VUY:
	case CFHD_PIXEL_FORMAT_YUY2:
	case CFHD_PIXEL_FORMAT_V210:
	case CFHD_PIXEL_FORMAT_YU64:
		if (fileReader->ReadFrame(frameBuffer, framePitch,pixelFormat)) {
			return CFHD_ERROR_OKAY;
		}
		break;

	default:
		return CFHD_ERROR_BADFORMAT;
		break;
	}

	// Could not read and unpack the frame
	return CFHD_ERROR_READ_FAILURE;
}

// Return the frame width from the DPX file header
int DPXFrameWidth(DPXFileReaderRef fileRef)
{
	CDPXFileReader *fileReader = (CDPXFileReader *)fileRef;
	assert(fileRef != NULL);
	if (! (fileRef != NULL)) {
		return CFHD_ERROR_INVALID_ARGUMENT;
	}

	if (!fileReader->IsOpen()) {
		return CFHD_ERROR_BADFILE;
	}

	return (int)fileReader->FrameWidth();
}

// Return the frame height from the DPX file header
int DPXFrameHeight(DPXFileReaderRef fileRef)
{
	CDPXFileReader *fileReader = (CDPXFileReader *)fileRef;
	assert(fileRef != NULL);
	if (! (fileRef != NULL)) {
		return CFHD_ERROR_INVALID_ARGUMENT;
	}

	if (!fileReader->IsOpen()) {
		return CFHD_ERROR_BADFILE;
	}

	return (int)fileReader->FrameHeight();
}

// Return an array of pixel formats supported by the DPX file reader
CFHD_Error DPXGetPixelFormats(DPXFileReaderRef fileRef,
							  CFHD_PixelFormat *pixelFormatArray,
							  int pixelFormatArrayLength,
							  int *actualPixelFormatCountOut)
{
	if (actualPixelFormatCountOut) {
		*actualPixelFormatCountOut = 0;
	}

	CDPXFileReader *fileReader = (CDPXFileReader *)fileRef;
	assert(fileRef != NULL);
	if (! (fileRef != NULL)) {
		return CFHD_ERROR_INVALID_ARGUMENT;
	}

	if (!fileReader->IsOpen()) {
		return CFHD_ERROR_BADFILE;
	}

	if (pixelFormatArrayLength > 0)
	{
		pixelFormatArray[0] = CFHD_PIXEL_FORMAT_B64A;
	}

	if (actualPixelFormatCountOut) {
		*actualPixelFormatCountOut = 1;
	}

	return CFHD_ERROR_OKAY;
}

// Return the preferred frame pitch for the specified pixel format
int DPXFramePitch(DPXFileReaderRef fileRef,
				  CFHD_PixelFormat pixelFormat)
{
	CDPXFileReader *fileReader = (CDPXFileReader *)fileRef;
	assert(fileRef != NULL);
	if (! (fileRef != NULL)) {
		return CFHD_ERROR_INVALID_ARGUMENT;
	}

	if (!fileReader->IsOpen()) {
		return CFHD_ERROR_BADFILE;
	}

	int framePitch;
	int pixelSize;

	switch (pixelFormat)
	{
	case CFHD_PIXEL_FORMAT_BGRA:
		pixelSize = 4;
		framePitch = (int)(fileReader->FrameWidth() * pixelSize);
		break;

	case CFHD_PIXEL_FORMAT_YUY2:
	case CFHD_PIXEL_FORMAT_2VUY:
		pixelSize = 2;
		framePitch = (int)(fileReader->FrameWidth() * pixelSize);
		break;

	case CFHD_PIXEL_FORMAT_V210:
		{
			size_t width48 = ((fileReader->FrameWidth()+47)/48)*48;
			framePitch = (int)(width48 * 8) / 3;	// 16 byte aligned
		}
		break;

	case CFHD_PIXEL_FORMAT_YU64:
		pixelSize = 4;
		framePitch = (int)(fileReader->FrameWidth() * pixelSize);
		break;

	case CFHD_PIXEL_FORMAT_AB10:
	case CFHD_PIXEL_FORMAT_AR10:
	case CFHD_PIXEL_FORMAT_RG30:
	case CFHD_PIXEL_FORMAT_R210:
	case CFHD_PIXEL_FORMAT_DPX0:
		pixelSize = 4;
		framePitch = (int)(fileReader->FrameWidth() * pixelSize);
		break;

	case CFHD_PIXEL_FORMAT_RG48:
		pixelSize = 6;
		framePitch = (int)(fileReader->FrameWidth() * pixelSize);
		break;

	case CFHD_PIXEL_FORMAT_B64A:
	case CFHD_PIXEL_FORMAT_RG64:
		pixelSize = 8;
		framePitch = (int)(fileReader->FrameWidth() * pixelSize);
		break;

	case CFHD_PIXEL_FORMAT_BYR4:
		pixelSize = 2;
		framePitch = (int)(fileReader->FrameWidth() * pixelSize);
		break;

	case CFHD_PIXEL_FORMAT_BYR5:
		pixelSize = 2; //1.5
		framePitch = (int)(fileReader->FrameWidth() * 3 / 2);
		break;

	default:
		framePitch = 0;
		break;
	}

	return framePitch;
}

// Close the DPX file
CFHD_Error DPXFileReaderClose(DPXFileReaderRef fileRef)
{
	CDPXFileReader *fileReader = (CDPXFileReader *)fileRef;
	assert(fileRef != NULL);
	if (! (fileRef != NULL)) {
		return CFHD_ERROR_INVALID_ARGUMENT;
	}

	delete fileReader;

	return CFHD_ERROR_OKAY;
}
