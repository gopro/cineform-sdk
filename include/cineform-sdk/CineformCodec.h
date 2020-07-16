#pragma once

#include <cstdint>

class CineformCodec
{
public:
   enum DecoderDownsamplingMode { None=1, HalfSize=2, QuarterSize=4 };
   enum CodecType { EncodeDecode, EncodeOnly, DecodeOnly };

   CineformCodec( int width,
                  int height,
                  int stride,
                  CodecType codecType = EncodeDecode,
                  DecoderDownsamplingMode downsamplingMode = None );
   virtual ~CineformCodec();

   int encodeFrame( const uint8_t* frameData,
                    int frameDataSize,
                    uint8_t* encodedData,
                    int encodedDataSize );

   void decodeFrame( const uint8_t* encodedData,
                     int encodedDataSize,
                     uint8_t* frameData,
                     int frameDataSize );

protected:
   bool canEncode() const;
   bool canDecode() const;

   const int                     _Width;
   const int                     _Height;
   const int                     _Stride;
   const int                     _ExpectedEncodeBufferSize;
   const int                     _ExpectedDecodeBufferSize;
   const CodecType               _CodecType;
   const DecoderDownsamplingMode _DownsamplingMode;
   void*                         _Encoder = nullptr;
   void*                         _Decoder = nullptr;
   bool                          _EncoderInitialized = false;
   bool                          _DecoderInitialized = false;
};