// $Header$

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

void WriteToDPX(char *pathnameTemplate,		// Format string with a decimal format specifier
				int frameNumber,			// Frame number
				uint16_t *bayerBuffer,		// Buffer of Bayer pixel data
				int bufferWidth,			// Width of buffer in values (not Bayer quads)
				int bufferHeight,			// Height of buffer in values (not Bayer quads)
				int bufferPitch,			// Number of bytes per row
				int bufferFormat,			// Pixel format
				int bayerFormat);			// Bayer format (if the pixel format is Bayer)

void WriteBayer(char *pathnameTemplate,		// Format string with a decimal format specifier
				int frameNumber,			// Frame number
				uint16_t *bayerBuffer,		// Buffer of Bayer pixel data
				int bufferWidth,			// Width of buffer in values (not Bayer quads)
				int bufferHeight,			// Height of buffer in values (not Bayer quads)
				int bufferPitch,			// Number of bytes per row
				int bayerFormat);			// Bayer format

void WriteARGB64(char *pathnameTemplate,	// Format string with a decimal format specifier
				 int frameNumber,			// Frame number
				 uint16_t *bayerBuffer,		// Buffer of Bayer pixel data
				 int bufferWidth,			// Width of buffer in values (not Bayer quads)
				 int bufferHeight,			// Height of buffer in values (not Bayer quads)
				 int bufferPitch);			// Number of bytes per row

void WriteRG48(char *pathnameTemplate,	// Format string with a decimal format specifier
				 int frameNumber,			// Frame number
				 uint16_t *bayerBuffer,		// Buffer of Bayer pixel data
				 int bufferWidth,			// Width of buffer in values (not Bayer quads)
				 int bufferHeight,			// Height of buffer in values (not Bayer quads)
				 int bufferPitch);			// Number of bytes per row

void WriteWP13(char *pathnameTemplate,	// Format string with a decimal format specifier
				 int frameNumber,			// Frame number
				 uint16_t *bayerBuffer,		// Buffer of Bayer pixel data
				 int bufferWidth,			// Width of buffer in values (not Bayer quads)
				 int bufferHeight,			// Height of buffer in values (not Bayer quads)
				 int bufferPitch);			// Number of bytes per row

void WriteW13A(char *pathnameTemplate,	// Format string with a decimal format specifier
				 int frameNumber,			// Frame number
				 uint16_t *bayerBuffer,		// Buffer of Bayer pixel data
				 int bufferWidth,			// Width of buffer in values (not Bayer quads)
				 int bufferHeight,			// Height of buffer in values (not Bayer quads)
				 int bufferPitch);			// Number of bytes per row

void WriteV210(char *pathnameTemplate,	// Format string with a decimal format specifier
				 int frameNumber,			// Frame number
				 uint16_t *bayerBuffer,		// Buffer of Bayer pixel data
				 int bufferWidth,			// Width of buffer in values (not Bayer quads)
				 int bufferHeight,			// Height of buffer in values (not Bayer quads)
				 int bufferPitch);			// Number of bytes per row

void WriteYUY2(char *pathnameTemplate,	// Format string with a decimal format specifier
				 int frameNumber,			// Frame number
				 uint16_t *bayerBuffer,		// Buffer of Bayer pixel data
				 int bufferWidth,			// Width of buffer in values (not Bayer quads)
				 int bufferHeight,			// Height of buffer in values (not Bayer quads)
				 int bufferPitch);	

void WriteYU64(char *pathnameTemplate,	// Format string with a decimal format specifier
				 int frameNumber,			// Frame number
				 uint16_t *bayerBuffer,		// Buffer of Bayer pixel data
				 int bufferWidth,			// Width of buffer in values (not Bayer quads)
				 int bufferHeight,			// Height of buffer in values (not Bayer quads)
				 int bufferPitch);			// Number of bytes per row

void Write2VUY(char *pathnameTemplate,	// Format string with a decimal format specifier
				 int frameNumber,			// Frame number
				 uint16_t *bayerBuffer,		// Buffer of Bayer pixel data
				 int bufferWidth,			// Width of buffer in values (not Bayer quads)
				 int bufferHeight,			// Height of buffer in values (not Bayer quads)
				 int bufferPitch);			// Number of bytes per row

void WriteARGB32(char *pathnameTemplate,	// Format string with a decimal format specifier
				 int frameNumber,			// Frame number
				 uint16_t *bayerBuffer,		// Buffer of Bayer pixel data
				 int bufferWidth,			// Width of buffer in values (not Bayer quads)
				 int bufferHeight,			// Height of buffer in values (not Bayer quads)
				 int bufferPitch);			// Number of bytes per row

void WriteRGB24(char *pathnameTemplate,	// Format string with a decimal format specifier
				 int frameNumber,			// Frame number
				 uint16_t *bayerBuffer,		// Buffer of Bayer pixel data
				 int bufferWidth,			// Width of buffer in values (not Bayer quads)
				 int bufferHeight,			// Height of buffer in values (not Bayer quads)
				 int bufferPitch);			// Number of bytes per row

void WriteR408(char *pathnameTemplate,	// Format string with a decimal format specifier
				 int frameNumber,			// Frame number
				 uint16_t *bayerBuffer,		// Buffer of Bayer pixel data
				 int bufferWidth,			// Width of buffer in values (not Bayer quads)
				 int bufferHeight,			// Height of buffer in values (not Bayer quads)
				 int bufferPitch);			// Number of bytes per row

void WriteV408(char *pathnameTemplate,	// Format string with a decimal format specifier
				 int frameNumber,			// Frame number
				 uint16_t *bayerBuffer,		// Buffer of Bayer pixel data
				 int bufferWidth,			// Width of buffer in values (not Bayer quads)
				 int bufferHeight,			// Height of buffer in values (not Bayer quads)
				 int bufferPitch);			// Number of bytes per row

void WriteDPX0(char *pathnameTemplate,	// Format string with a decimal format specifier
				 int frameNumber,			// Frame number
				 uint16_t *bayerBuffer,		// Buffer of Bayer pixel data
				 int bufferWidth,			// Width of buffer in values (not Bayer quads)
				 int bufferHeight,			// Height of buffer in values (not Bayer quads)
				 int bufferPitch);			// Number of bytes per row
				 
void WriteR210(char *pathnameTemplate,	// Format string with a decimal format specifier
				 int frameNumber,			// Frame number
				 uint16_t *bayerBuffer,		// Buffer of Bayer pixel data
				 int bufferWidth,			// Width of buffer in values (not Bayer quads)
				 int bufferHeight,			// Height of buffer in values (not Bayer quads)
				 int bufferPitch);			// Number of bytes per row

void WriteRG30(char *pathnameTemplate,	// Format string with a decimal format specifier
				 int frameNumber,			// Frame number
				 uint16_t *bayerBuffer,		// Buffer of Bayer pixel data
				 int bufferWidth,			// Width of buffer in values (not Bayer quads)
				 int bufferHeight,			// Height of buffer in values (not Bayer quads)
				 int bufferPitch);			// Number of bytes per row

void WriteAR10(char *pathnameTemplate,	// Format string with a decimal format specifier
				 int frameNumber,			// Frame number
				 uint16_t *bayerBuffer,		// Buffer of Bayer pixel data
				 int bufferWidth,			// Width of buffer in values (not Bayer quads)
				 int bufferHeight,			// Height of buffer in values (not Bayer quads)
				 int bufferPitch);			// Number of bytes per row

#ifdef __cplusplus
}
#endif
