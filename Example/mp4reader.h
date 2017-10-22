/*! @file mp4reader.h
*
*  @brief Way Too Crude MP4|MOV reader 
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

#ifndef _MP4READER_H
#define _MP4READER_H

#ifdef __cplusplus
extern "C" {
#endif

typedef struct media_header
{
	uint8_t version_flags[4];
	uint32_t creation_time;
	uint32_t modification_time;
	uint32_t time_scale;
	uint32_t duration;
	uint16_t language;
	uint16_t quality;
} media_header;


typedef struct SampleToChunk
{
	uint32_t chunk_num;
	uint32_t samples;
	uint32_t id;
} SampleToChunk;


typedef struct videoobject
{
	uint32_t *metasizes;
	uint32_t metasize_count;
	uint64_t *metaoffsets;
	SampleToChunk *metastsc;
	uint32_t metastsc_count;
	uint32_t indexcount;
	float videolength;
	float metadatalength;
	uint32_t clockdemon, clockcount;
	uint32_t trak_clockdemon, trak_clockcount;
	uint32_t meta_clockdemon, meta_clockcount;
	uint32_t basemetadataduration;
	uint32_t basemetadataoffset;
	FILE *mediafp;
} videoobject;

#define MAKEID(a,b,c,d)			(((d&0xff)<<24)|((c&0xff)<<16)|((b&0xff)<<8)|(a&0xff))
#define STR2FOURCC(s)			((s[0]<<0)|(s[1]<<8)|(s[2]<<16)|(s[3]<<24))

#define BYTESWAP64(a)			(((a&0xff)<<56)|((a&0xff00)<<40)|((a&0xff0000)<<24)|((a&0xff000000)<<8) | ((a>>56)&0xff)|((a>>40)&0xff00)|((a>>24)&0xff0000)|((a>>8)&0xff000000) )
#define BYTESWAP32(a)			(((a&0xff)<<24)|((a&0xff00)<<8)|((a>>8)&0xff00)|((a>>24)&0xff))
#define BYTESWAP16(a)			((((a)>>8)&0xff)|(((a)<<8)&0xff00))
#define NOSWAP8(a)				(a)

#define MOV_TRAK_TYPE		MAKEID('v', 'i', 'd', 'e')		// track is the type for video
#define MOV_TRAK_SUBTYPE	MAKEID('C', 'F', 'H', 'D')		// subtype is CineForm HD
#define AVI_TRAK_TYPE		MAKEID('v', 'i', 'd', 's')		// track is the type for video
#define AVI_TRAK_SUBTYPE	MAKEID('c', 'f', 'h', 'd')		// subtype is CineForm HD

#define NESTSIZE(x) { int i = nest; while (i > 0 && nestsize[i] > 0) { nestsize[i] -= x; if(nestsize[i]>=0 && nestsize[i] <= 8) { nestsize[i]=0; nest--; } i--; } }

#define VALID_FOURCC(a)	(((((a>>24)&0xff)>='a'&&((a>>24)&0xff)<='z') || (((a>>24)&0xff)>='A'&&((a>>24)&0xff)<='Z') || (((a>>24)&0xff)>='0'&&((a>>24)&0xff)<='9') || (((a>>24)&0xff)==' ') ) && \
						( (((a>>16)&0xff)>='a'&&((a>>24)&0xff)<='z') || (((a>>16)&0xff)>='A'&&((a>>16)&0xff)<='Z') || (((a>>16)&0xff)>='0'&&((a>>16)&0xff)<='9') || (((a>>16)&0xff)==' ') ) && \
						( (((a>>8)&0xff)>='a'&&((a>>24)&0xff)<='z') || (((a>>8)&0xff)>='A'&&((a>>8)&0xff)<='Z') || (((a>>8)&0xff)>='0'&&((a>>8)&0xff)<='9') || (((a>>8)&0xff)==' ') ) && \
						( (((a>>0)&0xff)>='a'&&((a>>24)&0xff)<='z') || (((a>>0)&0xff)>='A'&&((a>>0)&0xff)<='Z') || (((a>>0)&0xff)>='0'&&((a>>0)&0xff)<='9') || (((a>>0)&0xff)==' ') )) 

void *OpenMP4Source(char *filename, uint32_t traktype, uint32_t subtype);
void *OpenAVISource(char *filename, uint32_t traktype, uint32_t subtype);
void CloseSource(void *handle);
float GetDuration(void *handle);
uint32_t GetNumberPayloads(void *handle);
uint32_t *GetPayload(void *handle, uint32_t *lastpayload, uint32_t index);
void FreePayload(uint32_t *lastpayload);
uint32_t GetPayloadSize(void *handle, uint32_t index);
uint32_t GetPayloadTime(void *handle, uint32_t index, float *in, float *out); //MP4 timestamps for the payload

#ifdef __cplusplus
}
#endif

#endif
