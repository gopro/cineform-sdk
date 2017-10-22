/*

readavi - Copyright 2004-2013 by Michael Kohn <mike@mikekohn.net>
For more info please visit http://www.mikekohn.net/

This code falls under the BSD license.

*/

struct avi_header_t
{
  int TimeBetweenFrames;
  int MaximumDataRate;
  int PaddingGranularity;
  int Flags;
  int TotalNumberOfFrames;
  int NumberOfInitialFrames;
  int NumberOfStreams;
  int SuggestedBufferSize;
  int Width;
  int Height;
  int TimeScale;
  int DataRate;
  int StartTime;
  int DataLength;
};

struct stream_header_t
{
  char DataType[5];
  char DataHandler[5];
  int Flags;
  int Priority;
  int InitialFrames;
  int TimeScale;
  int DataRate;
  int StartTime;
  int DataLength;
  int SuggestedBufferSize;
  int Quality;
  int SampleSize;
};

struct stream_format_t
{
  int header_size;
  int image_width;
  int image_height;
  int number_of_planes;
  int bits_per_pixel;
  int compression_type;
  int image_size_in_bytes;
  int x_pels_per_meter;
  int y_pels_per_meter;
  int colors_used;
  int colors_important;
  int *palette;
};

struct stream_header_auds_t
{
  int format_type;
  int number_of_channels;
  int sample_rate;
  int bytes_per_second;
  int block_size_of_data;
  int bits_per_sample;
  int byte_count_extended;
};

struct stream_format_auds_t
{
  int header_size;
  int format;
  int channels;
  int samples_per_second;
  int bytes_per_second;
  int block_size_of_data;
  int bits_per_sample;
  int extended_size;
};

struct index_entry_t
{
  char ckid[5];
  int dwFlags;
  int dwChunkOffset;
  int dwChunkLength;
};



