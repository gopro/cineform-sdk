#include "CineformCodec.h"

#include "CFHDDecoder.h"
#include "CFHDEncoder.h"

#include <cstring>
#include <stdexcept>

namespace
{
   CFHD_DecodedResolution decodedResolutionFromDecoderDownsamplingMode( CineformCodec::DecoderDownsamplingMode mode )
   {
      switch ( mode )
      {
         case CineformCodec::QuarterSize: return CFHD_DECODED_RESOLUTION_QUARTER;
         case CineformCodec::HalfSize: return CFHD_DECODED_RESOLUTION_HALF;
         default: return CFHD_DECODED_RESOLUTION_FULL;
      }
   }
}

CineformCodec::CineformCodec( int width, int height, int stride, CodecType codecType, DecoderDownsamplingMode downsamplingMode )
   : _Width( width )
   , _Height( height )
   , _Stride( stride )
   , _ExpectedBufferSize( ( stride / downsamplingMode ) * ( height / downsamplingMode ) )
   , _CodecType( codecType )
   , _DownsamplingMode( downsamplingMode )
{
   if ( canEncode() && ::CFHD_OpenEncoder( &_Encoder, nullptr ) != CFHD_ERROR_OKAY )
      throw std::runtime_error( "Error opening Cineform encoder" );

   if ( canDecode() && ::CFHD_OpenDecoder( &_Decoder, nullptr ) != CFHD_ERROR_OKAY )
      throw std::runtime_error( "Error opening Cineform decoder" );
}

CineformCodec::~CineformCodec()
{
   if ( _Encoder != nullptr )
   {
      ::CFHD_CloseEncoder( _Encoder );
      _Encoder = nullptr;
   }

   if ( _Decoder != nullptr )
   {
      ::CFHD_CloseDecoder( _Decoder );
      _Decoder = nullptr;
   }
}

int CineformCodec::encodeFrame( const uint8_t* frameData, int frameDataSize, uint8_t* encodedData, int encodedDataSize )
{
   if ( !canEncode() )
      throw std::runtime_error( "Attempting to encode with a Cineform codec not created for encoding" );

   if ( frameDataSize < _ExpectedBufferSize )
      throw std::runtime_error( "Passed too-small frame data to Cineform encoding" );

   if ( !_EncoderInitialized )
   {
      CFHD_PixelFormat inputPixelFormat = CFHD_PIXEL_FORMAT_BGRA;
      CFHD_EncodedFormat outputFormat = CFHD_ENCODED_FORMAT_YUV_422;
      CFHD_EncodingQuality quality = CFHD_ENCODING_QUALITY_DEFAULT;
      if ( ::CFHD_PrepareToEncode( _Encoder, _Width, _Height, inputPixelFormat, outputFormat, kCFHDEncodingFlagsNone, quality ) != CFHD_ERROR_OKAY )
         throw std::runtime_error( "Error initializing Cineform for encoding" );

      _EncoderInitialized = true;
   }

   if ( ::CFHD_EncodeSample( _Encoder, const_cast<uint8_t*>( frameData ), _Stride ) != CFHD_ERROR_OKAY )
      throw std::runtime_error( "Error encoding Cineform sample" );

   void* compressedSample = nullptr;
   size_t compressedSize = 0;
   if ( ::CFHD_GetSampleData( _Encoder, &compressedSample, &compressedSize ) != CFHD_ERROR_OKAY )
      throw std::runtime_error( "Error retrieving encoded Cineform sample" );
   if ( compressedSize > encodedDataSize )
      throw std::runtime_error( "Output buffer for encoded Cineform sample is too small" );

   std::memcpy( encodedData, compressedSample, compressedSize );

   return static_cast<int>( compressedSize );
}

void CineformCodec::decodeFrame( const uint8_t* encodedData, int encodedDataSize, uint8_t* frameData, int frameDataSize )
{
   if ( !canDecode() )
      throw std::runtime_error( "Attempting to decode with a Cineform codec not created for decoding" );

   if ( frameDataSize < _ExpectedBufferSize )
      throw std::runtime_error( "Passed too-small frame data to Cineform decoding" );

   if ( !_DecoderInitialized )
   {
      int actualWidth, actualHeight;
      CFHD_PixelFormat actualPixelFormatOut;
      CFHD_PixelFormat outputPixelFormat = CFHD_PIXEL_FORMAT_BGRA;
      CFHD_DecodedResolution decodedResolution = decodedResolutionFromDecoderDownsamplingMode( _DownsamplingMode );
      if ( ::CFHD_PrepareToDecode( _Decoder, 0, 0, outputPixelFormat,
                                   decodedResolution, kCFHDDecodingFlagsNone,
                                   frameData,
                                   512,   // Docs say we can just pass in 512
                                   &actualWidth, &actualHeight, &actualPixelFormatOut ) != CFHD_ERROR_OKAY )
         throw std::runtime_error( "Error initializing Cineform for decoding" );

      _DecoderInitialized = true;
   }

   if ( ::CFHD_DecodeSample( _Decoder, const_cast<uint8_t *>( encodedData ), encodedDataSize,
                             frameData, _Stride ) != CFHD_ERROR_OKAY )
      throw std::runtime_error( "Error decoding Cineform sample" );
}

bool CineformCodec::canEncode() const
{
   return _CodecType == EncodeDecode || _CodecType == EncodeOnly;
}

bool CineformCodec::canDecode() const
{
   _CodecType == EncodeDecode || _CodecType == DecodeOnly;
}