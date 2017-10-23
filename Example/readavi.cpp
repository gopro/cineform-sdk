/*

readavi - Copyright 2004-2013 by Michael Kohn <mike@mikekohn.net>
For more info please visit http://www.mikekohn.net/

This code falls under the BSD license.

*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#include "avi.h"
#include "fileio.h"
#include "mp4reader.h"

#ifdef _WINDOWS
#define STRINGCASECOMPARE  _stricmp
#else
#define STRINGCASECOMPARE  strcasecmp
#endif


#define PRINT_AVI_STRUCTURE		0

uint64_t movi_offset = 0;

/*
static int skip_chunk(FILE *in)
{
char chunk_id[5];
int chunk_size;
int end_of_chunk;

  read_chars(in,chunk_id,4);
  chunk_size=read_long(in);

  printf("Uknown Chunk at %d\n",(int)ftell(in));
  printf("-------------------------------\n");
  printf("             chunk_id: %s\n",chunk_id);
  printf("           chunk_size: %d\n",chunk_size);
  printf("\n");

  end_of_chunk=ftell(in)+chunk_size;
  if ((end_of_chunk%4)!=0)
  {
    end_of_chunk=end_of_chunk+(4-(end_of_chunk%4));
  }

  fseek(in,end_of_chunk,SEEK_SET);
 
  return 0;
}
*/

static int hex_dump_chunk(FILE *in, int chunk_len)
{
char chars[17];
int ch,n;

  chars[16]=0;

  for (n=0; n<chunk_len; n++)
  {
    if ((n%16)==0)
    {
      if (n!=0) printf("%s\n", chars);
      printf("      ");
      memset(chars, ' ', 16);
    }
    ch=getc(in);
    if (ch==EOF) break;
    printf("%02x ", ch); 
    if (ch>=' ' && ch<=126)
    { chars[n%16]=ch; }
      else
    { chars[n%16]='.'; }
  }

  if ((n%16)!=0)
  {
    for (ch=n%16; ch<16; ch++) { printf("   "); }
  }
  printf("%s\n", chars);

  return 0;
}

static int parse_idx1(void *hdl, FILE *in, int chunk_len)
{
	videoobject *mp4 = (videoobject *)hdl;
	if (mp4 == NULL) return 0;

struct index_entry_t index_entry;
int t,framenum = 0;

#if PRINT_AVI_STRUCTURE
  printf("      IDX1\n");
  printf("      -------------------------------\n");
  printf("      ckid   dwFlags         dwChunkOffset        dwChunkLength\n");
#endif

  for (t=0; t<chunk_len/16; t++)
  {
    read_chars(in,index_entry.ckid,4);
    index_entry.dwFlags=read_long(in);
    index_entry.dwChunkOffset=read_long(in);
    index_entry.dwChunkLength=read_long(in);

	if (0 == strcmp(index_entry.ckid, "00dc") || 0 == strcmp(index_entry.ckid, "00db")) //video frame compressed or baseband
	{
		if (mp4->metaoffsets)
		{
			mp4->metaoffsets[framenum] = movi_offset + 4 + (uint64_t)index_entry.dwChunkOffset;
			mp4->metasizes[framenum] = (uint64_t)index_entry.dwChunkLength;
			framenum++;
		}
	}


	mp4->indexcount = framenum;
	mp4->trak_clockcount = mp4->meta_clockcount = mp4->clockcount = mp4->indexcount * mp4->basemetadataduration;
	mp4->videolength = mp4->metadatalength = (float)mp4->clockcount / (float)mp4->clockdemon;


#if PRINT_AVI_STRUCTURE
    printf("      %s   0x%08x      0x%08x           0x%08x\n",
             index_entry.ckid,
             index_entry.dwFlags,
             index_entry.dwChunkOffset,
             index_entry.dwChunkLength);
#endif
  }

  printf("\n");

  return 0;
}

static int read_avi_header(void *hdl, FILE *in,struct avi_header_t *avi_header)
{
  videoobject *mp4 = (videoobject *)hdl;
  if (mp4 == NULL) return 0;

  avi_header->TimeBetweenFrames=read_long(in);
  avi_header->MaximumDataRate=read_long(in);
  avi_header->PaddingGranularity=read_long(in);
  avi_header->Flags=read_long(in);
  avi_header->TotalNumberOfFrames=read_long(in);
  avi_header->NumberOfInitialFrames=read_long(in);
  avi_header->NumberOfStreams=read_long(in);
  avi_header->SuggestedBufferSize=read_long(in);
  avi_header->Width=read_long(in);
  avi_header->Height=read_long(in);
  avi_header->TimeScale=read_long(in);
  avi_header->DataRate=read_long(in);
  avi_header->StartTime=read_long(in);
  avi_header->DataLength=read_long(in);

#if PRINT_AVI_STRUCTURE
  printf("         offset=0x%lx\n",offset);
  printf("             TimeBetweenFrames: %d\n",avi_header->TimeBetweenFrames);
  printf("               MaximumDataRate: %d\n",avi_header->MaximumDataRate);
  printf("            PaddingGranularity: %d\n",avi_header->PaddingGranularity);
  printf("                         Flags: %d\n",avi_header->Flags);
  printf("           TotalNumberOfFrames: %d\n",avi_header->TotalNumberOfFrames);
  printf("         NumberOfInitialFrames: %d\n",avi_header->NumberOfInitialFrames);
  printf("               NumberOfStreams: %d\n",avi_header->NumberOfStreams);
  printf("           SuggestedBufferSize: %d\n",avi_header->SuggestedBufferSize);
  printf("                         Width: %d\n",avi_header->Width);
  printf("                        Height: %d\n",avi_header->Height);
  printf("                     TimeScale: %d\n",avi_header->TimeScale);
  printf("                      DataRate: %d\n",avi_header->DataRate);
  printf("                     StartTime: %d\n",avi_header->StartTime);
  printf("                    DataLength: %d\n",avi_header->DataLength);
#endif



  mp4->indexcount = avi_header->TotalNumberOfFrames;
  mp4->basemetadataduration = avi_header->TimeBetweenFrames;
  mp4->trak_clockdemon = mp4->meta_clockdemon = mp4->clockdemon = 1000000;
  mp4->trak_clockcount = mp4->meta_clockcount = mp4->clockcount = mp4->indexcount * mp4->basemetadataduration;
  mp4->videolength = mp4->metadatalength = (float)mp4->clockcount / (float)mp4->clockdemon;

  mp4->metasizes = (uint32_t *)malloc(mp4->indexcount * 4);
  mp4->metasize_count = mp4->indexcount;
  mp4->metaoffsets = (uint64_t *)malloc(mp4->indexcount * 8);

  return 0;
}

static void print_data_handler(unsigned char *handler)
{
int t;

  for (t=0; t<4; t++)
  {
    if ((handler[t]>='a' && handler[t]<='z') ||
        (handler[t]>='A' && handler[t]<='Z') ||
        (handler[t]>='0' && handler[t]<='9'))
    {
      printf("%c",handler[t]);
    }
      else
    {
      printf("[0x%02x]",handler[t]);
    }
  }
}

static int read_stream_header(FILE *in,struct stream_header_t *stream_header)
{
long offset=ftell(in);

  read_chars(in,stream_header->DataType,4);
  read_chars(in,stream_header->DataHandler,4);
  stream_header->Flags=read_long(in);
  stream_header->Priority=read_long(in);
  stream_header->InitialFrames=read_long(in);
  stream_header->TimeScale=read_long(in);
  stream_header->DataRate=read_long(in);
  stream_header->StartTime=read_long(in);
  stream_header->DataLength=read_long(in);
  stream_header->SuggestedBufferSize=read_long(in);
  stream_header->Quality=read_long(in);
  stream_header->SampleSize=read_long(in);

#if PRINT_AVI_STRUCTURE
  printf("            offset=0x%lx\n",offset);
  printf("                      DataType: %s\n",stream_header->DataType);
  printf("                   DataHandler: ");
  print_data_handler((unsigned char *)stream_header->DataHandler);
  printf("\n");
  printf("                         Flags: %d\n",stream_header->Flags);
  printf("                      Priority: %d\n",stream_header->Priority);
  printf("                 InitialFrames: %d\n",stream_header->InitialFrames);
  printf("                     TimeScale: %d\n",stream_header->TimeScale);
  printf("                      DataRate: %d\n",stream_header->DataRate);
  printf("                     StartTime: %d\n",stream_header->StartTime);
  printf("                    DataLength: %d\n",stream_header->DataLength);
  printf("           SuggestedBufferSize: %d\n",stream_header->SuggestedBufferSize);
  printf("                       Quality: %d\n",stream_header->Quality);
  printf("                    SampleSize: %d\n",stream_header->SampleSize);
#endif

  return 0;
}

static int read_stream_format(FILE *in,struct stream_format_t *stream_format)
{
int t,r,g,b;
long offset=ftell(in);

  stream_format->header_size=read_long(in);
  stream_format->image_width=read_long(in);
  stream_format->image_height=read_long(in);
  stream_format->number_of_planes=read_word(in);
  stream_format->bits_per_pixel=read_word(in);
  stream_format->compression_type=read_long(in);
  stream_format->image_size_in_bytes=read_long(in);
  stream_format->x_pels_per_meter=read_long(in);
  stream_format->y_pels_per_meter=read_long(in);
  stream_format->colors_used=read_long(in);
  stream_format->colors_important=read_long(in);
  stream_format->palette=0;

  if (stream_format->colors_important!=0)
  {
    stream_format->palette=(int *)malloc(stream_format->colors_important*sizeof(int));
    for (t=0; t<stream_format->colors_important; t++)
    {
      b=getc(in);
      g=getc(in);
      r=getc(in);
      stream_format->palette[t]=(r<<16)+(g<<8)+b;
    }
  }

#if PRINT_AVI_STRUCTURE
  printf("            offset=0x%lx\n",offset);
  printf("                   header_size: %d\n",stream_format->header_size);
  printf("                   image_width: %d\n",stream_format->image_width);
  printf("                  image_height: %d\n",stream_format->image_height);
  printf("              number_of_planes: %d\n",stream_format->number_of_planes);
  printf("                bits_per_pixel: %d\n",stream_format->bits_per_pixel);
  printf("              compression_type: %04x (%c%c%c%c)\n",stream_format->compression_type,
                        ((stream_format->compression_type)&255),
                        ((stream_format->compression_type>>8)&255),
                        ((stream_format->compression_type>>16)&255),
                        ((stream_format->compression_type>>24)&255));
  printf("           image_size_in_bytes: %d\n",stream_format->image_size_in_bytes);
  printf("              x_pels_per_meter: %d\n",stream_format->x_pels_per_meter);
  printf("              y_pels_per_meter: %d\n",stream_format->y_pels_per_meter);
  printf("                   colors_used: %d\n",stream_format->colors_used);
  printf("              colors_important: %d\n",stream_format->colors_important);
#endif

  return 0;
}

static int read_stream_format_auds(FILE *in, struct stream_format_auds_t *stream_format)
{
long offset=ftell(in);

  stream_format->format=read_word(in);
  stream_format->channels=read_word(in);
  stream_format->samples_per_second=read_long(in);
  stream_format->bytes_per_second=read_long(in);
  int block_align=read_word(in);
  stream_format->block_size_of_data=read_word(in);
  stream_format->bits_per_sample=read_word(in);
  //stream_format->extended_size=read_word(in);

#if PRINT_AVI_STRUCTURE
  printf("            offset=0x%lx\n",offset);
  printf("                        format: %d\n",stream_format->format);
  printf("                      channels: %d\n",stream_format->channels);
  printf("            samples_per_second: %d\n",stream_format->samples_per_second);
  printf("              bytes_per_second: %d\n",stream_format->bytes_per_second);
  printf("                   block_align: %d\n",block_align);
  printf("            block_size_of_data: %d\n",stream_format->block_size_of_data);
  printf("               bits_per_sample: %d\n",stream_format->bits_per_sample);
#endif

  return 0;
}

static int parse_hdrl_list(FILE *in,struct avi_header_t *avi_header, struct stream_header_t *stream_header, struct stream_format_t *stream_format)
{
struct stream_format_auds_t stream_format_auds;
struct stream_header_t stream_header_auds;
char chunk_id[5];
int chunk_size;
char chunk_type[5];
int end_of_chunk;
int next_chunk;
long offset=ftell(in);
int stream_type=0;     // 0=video 1=sound

  read_chars(in,chunk_id,4);
  chunk_size=read_long(in);
  read_chars(in,chunk_type,4);

#if PRINT_AVI_STRUCTURE
  printf("      AVI Header LIST (id=%s size=%d type=%s offset=0x%lx)\n",chunk_id,chunk_size,chunk_type,offset);
  printf("      {\n");
#endif

  end_of_chunk=ftell(in)+chunk_size-4;
  if ((end_of_chunk%4)!=0)
  {
//printf("Adjusting end of chunk %d\n", end_of_chunk);
    //end_of_chunk=end_of_chunk+(4-(end_of_chunk%4));
//printf("Adjusting end of chunk %d\n", end_of_chunk);
  }

  if (strcmp(chunk_id,"JUNK")==0)
  {
    fseek(in,end_of_chunk,SEEK_SET);
#if PRINT_AVI_STRUCTURE
    printf("      }\n");
#endif
    return 0;
  }

  while (ftell(in)<end_of_chunk)
  {
    long offset=ftell(in);
    read_chars(in,chunk_type,4);
    chunk_size=read_long(in);
    next_chunk=ftell(in)+chunk_size;
    if ((chunk_size%4)!=0)
    {
//printf("Chunk size not a multiple of 4?\n");
      //chunk_size=chunk_size+(4-(chunk_size%4));
    }

#if PRINT_AVI_STRUCTURE
    printf("         %.4s (size=%d offset=0x%lx)\n",chunk_type,chunk_size,offset);
    printf("         {\n");
#endif
    if (STRINGCASECOMPARE("strh",chunk_type)==0)
    {
      long marker=ftell(in);
      char buffer[5];
      read_chars(in,buffer,4); 
      fseek(in,marker,SEEK_SET);

      if (strcmp(buffer, "vids")==0)
      {
        stream_type=0;
        read_stream_header(in,stream_header);
      }
        else
      if (strcmp(buffer, "auds")==0)
      {
        stream_type=1;
        read_stream_header(in,&stream_header_auds);
      }
        else
      {
        printf("Unknown stream type %s\n", buffer); 
        return -1;
      }
    }
      else
    if (STRINGCASECOMPARE("strf",chunk_type)==0)
    {
      if (stream_type==0)
      {
        read_stream_format(in,stream_format);
      }
      else
      {
        read_stream_format_auds(in,&stream_format_auds);
      }
    }
      else
    if (STRINGCASECOMPARE("strd",chunk_type)==0)
    {

    }
      else
    {
#if PRINT_AVI_STRUCTURE
      printf("            Unknown chunk type: %s\n",chunk_type);
#endif
      // skip_chunk(in);
    }

#if PRINT_AVI_STRUCTURE
    printf("         }\n");
#endif
    fseek(in,next_chunk,SEEK_SET);
  }

//printf("@@@@ %ld %d\n", ftell(in), end_of_chunk);
#if PRINT_AVI_STRUCTURE
  printf("      }\n");
#endif
  fseek(in,end_of_chunk,SEEK_SET);

  return 0;
}

static int parse_hdrl(void *hdl, FILE *in,struct avi_header_t *avi_header, struct stream_header_t *stream_header, struct stream_format_t *stream_format, unsigned int size)
{
char chunk_id[5];
int chunk_size;
int end_of_chunk;
long offset=ftell(in);

  read_chars(in,chunk_id,4);
  chunk_size=read_long(in);

#if PRINT_AVI_STRUCTURE
  printf("      AVI Header Chunk (id=%s size=%d offset=0x%lx)\n",chunk_id,chunk_size,offset);
  printf("      {\n");
#endif

  end_of_chunk=ftell(in)+chunk_size;
  if ((end_of_chunk%4)!=0)
  {
    end_of_chunk=end_of_chunk+(4-(end_of_chunk%4));
  }

  read_avi_header(hdl, in,avi_header);

#if PRINT_AVI_STRUCTURE
  printf("      }\n");
#endif

  while(ftell(in)<offset+(long)size-4)
  {
    //printf("Should end at 0x%lx  0x%lx\n",offset+size,ftell(in));
    parse_hdrl_list(in,avi_header,stream_header,stream_format);
  }

  return 0;
}

static int parse_riff(void *hdl)
{
FILE *in;
char chunk_id[5];
int chunk_size;
char chunk_type[5];
int end_of_chunk,end_of_subchunk;
struct avi_header_t avi_header;
struct stream_header_t stream_header;
struct stream_format_t stream_format={0};
long offset;

videoobject *mp4 = (videoobject *)hdl;
if (mp4 == NULL) return 0;

in = mp4->mediafp;
offset= ftell(in);



  read_chars(in,chunk_id,4);
  chunk_size=read_long(in);
  read_chars(in,chunk_type,4);

#if PRINT_AVI_STRUCTURE
  printf("RIFF Chunk (id=%s size=%d type=%s offset=0x%lx)\n",chunk_id,chunk_size,chunk_type, offset);
  printf("{\n");
#endif

  if (STRINGCASECOMPARE("RIFF",chunk_id)!=0)
  {
    printf("Not a RIFF file.\n");
    return 1;
  }
    else
  if (STRINGCASECOMPARE("AVI ",chunk_type)!=0)
  {
    printf("Not an AVI file.\n");
    return 1;
  }

  end_of_chunk=ftell(in)+chunk_size-4;

  while (ftell(in)<end_of_chunk)
  {
    long offset=ftell(in);
    read_chars(in,chunk_id,4);
    chunk_size=read_long(in);
    end_of_subchunk=ftell(in)+chunk_size;

    if (STRINGCASECOMPARE("JUNK",chunk_id)==0 || STRINGCASECOMPARE("PAD ",chunk_id)==0)
    { chunk_type[0]=0; }
      else
    { read_chars(in,chunk_type,4); }

#if PRINT_AVI_STRUCTURE
    printf("   New Chunk (id=%s size=%d type=%s offset=0x%lx)\n",chunk_id,chunk_size,chunk_type,offset);
    printf("   {\n");
#endif

    fflush(stdout);

    if (STRINGCASECOMPARE("JUNK",chunk_id)==0 || STRINGCASECOMPARE("PAD ",chunk_id)==0)
    {
      if ((chunk_size%4)!=0)
      { chunk_size=chunk_size+(4-(chunk_size%4)); }

#if PRINT_AVI_STRUCTURE
      hex_dump_chunk(in, chunk_size);
#endif
    }
      else
    if (STRINGCASECOMPARE("INFO",chunk_type)==0)
    {
      if ((chunk_size%4)!=0)
      { chunk_size=chunk_size+(4-(chunk_size%4)); }

#if PRINT_AVI_STRUCTURE
      hex_dump_chunk(in, chunk_size);
#endif
    }
      else
    if (STRINGCASECOMPARE("hdrl",chunk_type)==0)
    {
      parse_hdrl(hdl, in,&avi_header,&stream_header,&stream_format, chunk_size);
      /* skip_chunk(in); */
    }
      else
    if (STRINGCASECOMPARE("movi",chunk_type)==0)
    {
		char nextid[5];
		long pos = ftell(in);
		movi_offset = (uint64_t)pos;
		read_chars(in, nextid, 4);
		fseek(in, pos, SEEK_SET);

		if (0 == strcmp("ix00", nextid)) // OpenDML AVI hack
			movi_offset = 4;

      // parse_movi(in);
    }
      else
    if (STRINGCASECOMPARE("idx1",chunk_id)==0)
    {
      fseek(in,ftell(in)-4,SEEK_SET);
      parse_idx1(hdl, in,chunk_size);
    }
      else
    {
#if PRINT_AVI_STRUCTURE
      printf("      Unknown chunk at %d (%4s)\n",(int)ftell(in)-8,chunk_type);
#endif
      if (chunk_size==0) break;
    }

    fseek(in,end_of_subchunk,SEEK_SET);
#if PRINT_AVI_STRUCTURE
    printf("   }\n");
#endif

  }

  if (stream_format.palette!=0)
  {
    free(stream_format.palette);
  }

#if PRINT_AVI_STRUCTURE
  printf("}\n");
#endif

  return 0;
}

void *OpenAVISource(char *filename, uint32_t traktype, uint32_t subtype)
{
	videoobject *mp4 = (videoobject *)malloc(sizeof(videoobject));
	if (mp4 == NULL) return NULL;

	memset(mp4, 0, sizeof(videoobject));


#ifdef _WINDOWS
	fopen_s(&mp4->mediafp, filename, "rb");
#else
	mp4->mediafp = fopen(filename, "rb");
#endif

  if (mp4->mediafp == 0)
  {
    printf("Could not open %s for input\n", filename);
    exit(1);
  }

  parse_riff((void *)mp4);

  return (void *)mp4;
}

