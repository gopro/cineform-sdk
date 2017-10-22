/*! @file utils.h

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


#pragma once

#include "CFHDTypes.h"

// required to support the orginal qBist code
typedef struct { int x, y; } Point;
typedef bool Boolean;

#define BYTESWAP32(a)			(((a&0xff)<<24)|((a&0xff00)<<8)|((a>>8)&0xff00)|((a>>24)&0xff))

unsigned int GetRand(unsigned int seed);
unsigned int myRand();


int ChannelsInPixelFormat(CFHD_PixelFormat pixelFormat);
int DepthInPixelFormat(CFHD_PixelFormat pixelFormat);
int InvertedPixelFormat(CFHD_PixelFormat pixelFormat);
int FramePitch4PixelFormat(CFHD_PixelFormat pixelFormat, int frameWidth);

void ExportPPM(char *filename, char *metadata, void *frameBuffer, int frameWidth, int frameHeight, int framePitch, CFHD_PixelFormat pixelFormat);

float PSNR(void *A, void *B, int width, int height, CFHD_PixelFormat pixelFormat, int scale);
