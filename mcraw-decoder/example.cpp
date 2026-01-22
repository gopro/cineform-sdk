/*
 * Copyright 2023 MotionCam
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <iostream>
#include <vector>
#include <cstring>
#include <cmath>
#include <cstdint>

#include <motioncam/Decoder.hpp>
#include <audiofile/AudioFile.h>

#include <CFHDEncoder.h>
#include <CFHDMetadata.h>
#include <CFHDTypes.h>

#define TINY_DNG_WRITER_IMPLEMENTATION
    #include <tinydng/tiny_dng_writer.h>
#undef TINY_DNG_WRITER_IMPLEMENTATION

// Bayer pattern definitions
enum BayerPattern {
    RGGB = 0,
    BGGR = 1,
    GRBG = 2,
    GBRG = 3
};

// Get pixel index for bayer data
static inline int bayerIndex(int x, int y, int width) {
    return y * width + x;
}

// Get bayer color at position (0=R, 1=G, 2=B)
static inline int bayerColor(BayerPattern pattern, int x, int y) {
    int x2 = x & 1;
    int y2 = y & 1;

    switch(pattern) {
        case RGGB: return (y2 == 0) ? (x2 == 0 ? 0 : 1) : (x2 == 0 ? 1 : 2);
        case BGGR: return (y2 == 0) ? (x2 == 0 ? 2 : 1) : (x2 == 0 ? 1 : 0);
        case GRBG: return (y2 == 0) ? (x2 == 0 ? 1 : 0) : (x2 == 0 ? 2 : 1);
        case GBRG: return (y2 == 0) ? (x2 == 0 ? 1 : 2) : (x2 == 0 ? 0 : 1);
        default: return 1;
    }
}

// Minimal MP4 writer (single video track, CFHD samples)
namespace {
struct Mp4Buffer {
    std::vector<uint8_t> data;
};

static inline void mp4WriteU8(Mp4Buffer& b, uint8_t v) { b.data.push_back(v); }
static inline void mp4WriteU16(Mp4Buffer& b, uint16_t v) {
    b.data.push_back(static_cast<uint8_t>((v >> 8) & 0xff));
    b.data.push_back(static_cast<uint8_t>(v & 0xff));
}
static inline void mp4WriteU32(Mp4Buffer& b, uint32_t v) {
    b.data.push_back(static_cast<uint8_t>((v >> 24) & 0xff));
    b.data.push_back(static_cast<uint8_t>((v >> 16) & 0xff));
    b.data.push_back(static_cast<uint8_t>((v >> 8) & 0xff));
    b.data.push_back(static_cast<uint8_t>(v & 0xff));
}
static inline void mp4WriteU64(Mp4Buffer& b, uint64_t v) {
    mp4WriteU32(b, static_cast<uint32_t>((v >> 32) & 0xffffffffu));
    mp4WriteU32(b, static_cast<uint32_t>(v & 0xffffffffu));
}
static inline void mp4WriteFourCC(Mp4Buffer& b, const char cc[4]) {
    mp4WriteU8(b, static_cast<uint8_t>(cc[0]));
    mp4WriteU8(b, static_cast<uint8_t>(cc[1]));
    mp4WriteU8(b, static_cast<uint8_t>(cc[2]));
    mp4WriteU8(b, static_cast<uint8_t>(cc[3]));
}
static inline size_t mp4StartAtom(Mp4Buffer& b, const char cc[4]) {
    size_t start = b.data.size();
    mp4WriteU32(b, 0); // placeholder for size
    mp4WriteFourCC(b, cc);
    return start;
}
static inline void mp4EndAtom(Mp4Buffer& b, size_t start) {
    size_t size = b.data.size() - start;
    b.data[start + 0] = static_cast<uint8_t>((size >> 24) & 0xff);
    b.data[start + 1] = static_cast<uint8_t>((size >> 16) & 0xff);
    b.data[start + 2] = static_cast<uint8_t>((size >> 8) & 0xff);
    b.data[start + 3] = static_cast<uint8_t>(size & 0xff);
}

static Mp4Buffer buildMoov(uint32_t timescale,
                           uint32_t duration,
                           uint32_t width,
                           uint32_t height,
                           const std::vector<uint32_t>& sampleSizes,
                           const std::vector<uint32_t>& sampleOffsets)
{
    Mp4Buffer b;

    size_t moov = mp4StartAtom(b, "moov");

    // mvhd
    {
        size_t mvhd = mp4StartAtom(b, "mvhd");
        mp4WriteU32(b, 0); // version/flags
        mp4WriteU32(b, 0); // creation
        mp4WriteU32(b, 0); // modification
        mp4WriteU32(b, timescale);
        mp4WriteU32(b, duration);
        mp4WriteU32(b, 0x00010000); // rate 1.0
        mp4WriteU16(b, 0x0100);     // volume 1.0
        mp4WriteU16(b, 0);          // reserved
        mp4WriteU32(b, 0);
        mp4WriteU32(b, 0);
        // matrix (identity)
        mp4WriteU32(b, 0x00010000);
        mp4WriteU32(b, 0);
        mp4WriteU32(b, 0);
        mp4WriteU32(b, 0);
        mp4WriteU32(b, 0x00010000);
        mp4WriteU32(b, 0);
        mp4WriteU32(b, 0);
        mp4WriteU32(b, 0);
        mp4WriteU32(b, 0x40000000);
        // pre-defined
        for (int i = 0; i < 6; i++) mp4WriteU32(b, 0);
        mp4WriteU32(b, 1); // next track id
        mp4EndAtom(b, mvhd);
    }

    // trak
    {
        size_t trak = mp4StartAtom(b, "trak");

        // tkhd
        {
            size_t tkhd = mp4StartAtom(b, "tkhd");
            mp4WriteU32(b, 0x00000007); // version/flags
            mp4WriteU32(b, 0); // creation
            mp4WriteU32(b, 0); // modification
            mp4WriteU32(b, 1); // track id
            mp4WriteU32(b, 0); // reserved
            mp4WriteU32(b, duration);
            mp4WriteU32(b, 0);
            mp4WriteU32(b, 0);
            mp4WriteU16(b, 0); // layer
            mp4WriteU16(b, 0); // alternate group
            mp4WriteU16(b, 0); // volume
            mp4WriteU16(b, 0); // reserved
            // matrix
            mp4WriteU32(b, 0x00010000);
            mp4WriteU32(b, 0);
            mp4WriteU32(b, 0);
            mp4WriteU32(b, 0);
            mp4WriteU32(b, 0x00010000);
            mp4WriteU32(b, 0);
            mp4WriteU32(b, 0);
            mp4WriteU32(b, 0);
            mp4WriteU32(b, 0x40000000);
            mp4WriteU32(b, width << 16);
            mp4WriteU32(b, height << 16);
            mp4EndAtom(b, tkhd);
        }

        // mdia
        {
            size_t mdia = mp4StartAtom(b, "mdia");

            // mdhd
            {
                size_t mdhd = mp4StartAtom(b, "mdhd");
                mp4WriteU32(b, 0); // version/flags
                mp4WriteU32(b, 0); // creation
                mp4WriteU32(b, 0); // modification
                mp4WriteU32(b, timescale);
                mp4WriteU32(b, duration);
                mp4WriteU16(b, 0x55c4); // und
                mp4WriteU16(b, 0);
                mp4EndAtom(b, mdhd);
            }

            // hdlr
            {
                size_t hdlr = mp4StartAtom(b, "hdlr");
                mp4WriteU32(b, 0); // version/flags
                mp4WriteU32(b, 0); // pre-defined
                mp4WriteFourCC(b, "vide");
                mp4WriteU32(b, 0);
                mp4WriteU32(b, 0);
                mp4WriteU32(b, 0);
                const char name[] = "VideoHandler";
                for (size_t i = 0; i < sizeof(name); i++) mp4WriteU8(b, static_cast<uint8_t>(name[i]));
                mp4EndAtom(b, hdlr);
            }

            // minf
            {
                size_t minf = mp4StartAtom(b, "minf");

                // vmhd
                {
                    size_t vmhd = mp4StartAtom(b, "vmhd");
                    mp4WriteU32(b, 0x00000001); // version/flags
                    mp4WriteU16(b, 0); // graphics mode
                    mp4WriteU16(b, 0);
                    mp4WriteU16(b, 0);
                    mp4WriteU16(b, 0);
                    mp4EndAtom(b, vmhd);
                }

                // dinf
                {
                    size_t dinf = mp4StartAtom(b, "dinf");
                    size_t dref = mp4StartAtom(b, "dref");
                    mp4WriteU32(b, 0); // version/flags
                    mp4WriteU32(b, 1); // entry count
                    size_t url = mp4StartAtom(b, "url ");
                    mp4WriteU32(b, 0x00000001); // version/flags (self-contained)
                    mp4EndAtom(b, url);
                    mp4EndAtom(b, dref);
                    mp4EndAtom(b, dinf);
                }

                // stbl
                {
                    size_t stbl = mp4StartAtom(b, "stbl");

                    // stsd
                    {
                        size_t stsd = mp4StartAtom(b, "stsd");
                        mp4WriteU32(b, 0); // version/flags
                        mp4WriteU32(b, 1); // entry count

                        size_t cfhd = mp4StartAtom(b, "CFHD");
                        // sample entry (video)
                        for (int i = 0; i < 6; i++) mp4WriteU8(b, 0);
                        mp4WriteU16(b, 1); // data reference index
                        mp4WriteU16(b, 0);
                        mp4WriteU16(b, 0);
                        mp4WriteU32(b, 0);
                        mp4WriteU32(b, 0);
                        mp4WriteU32(b, 0);
                        mp4WriteU16(b, static_cast<uint16_t>(width));
                        mp4WriteU16(b, static_cast<uint16_t>(height));
                        mp4WriteU32(b, 0x00480000); // horiz resolution 72dpi
                        mp4WriteU32(b, 0x00480000); // vert resolution
                        mp4WriteU32(b, 0);
                        mp4WriteU16(b, 1); // frame count
                        mp4WriteU8(b, 0); // compressor name length
                        for (int i = 0; i < 31; i++) mp4WriteU8(b, 0);
                        mp4WriteU16(b, 0x0018); // depth
                        mp4WriteU16(b, 0xffff); // pre-defined
                        mp4EndAtom(b, cfhd);
                        mp4EndAtom(b, stsd);
                    }

                    // stts
                    {
                        size_t stts = mp4StartAtom(b, "stts");
                        mp4WriteU32(b, 0); // version/flags
                        mp4WriteU32(b, 1); // entry count
                        mp4WriteU32(b, static_cast<uint32_t>(sampleSizes.size()));
                        uint32_t sampleDuration = (sampleSizes.empty()) ? 0 : (duration / static_cast<uint32_t>(sampleSizes.size()));
                        mp4WriteU32(b, sampleDuration);
                        mp4EndAtom(b, stts);
                    }

                    // stsc
                    {
                        size_t stsc = mp4StartAtom(b, "stsc");
                        mp4WriteU32(b, 0); // version/flags
                        mp4WriteU32(b, 1); // entry count
                        mp4WriteU32(b, 1); // first chunk
                        mp4WriteU32(b, 1); // samples per chunk
                        mp4WriteU32(b, 1); // sample desc index
                        mp4EndAtom(b, stsc);
                    }

                    // stsz
                    {
                        size_t stsz = mp4StartAtom(b, "stsz");
                        mp4WriteU32(b, 0); // version/flags
                        mp4WriteU32(b, 0); // sample size (0 = varies)
                        mp4WriteU32(b, static_cast<uint32_t>(sampleSizes.size()));
                        for (uint32_t s : sampleSizes) mp4WriteU32(b, s);
                        mp4EndAtom(b, stsz);
                    }

                    // stco
                    {
                        size_t stco = mp4StartAtom(b, "stco");
                        mp4WriteU32(b, 0); // version/flags
                        mp4WriteU32(b, static_cast<uint32_t>(sampleOffsets.size()));
                        for (uint32_t o : sampleOffsets) mp4WriteU32(b, o);
                        mp4EndAtom(b, stco);
                    }

                    mp4EndAtom(b, stbl);
                }

                mp4EndAtom(b, minf);
            }

            mp4EndAtom(b, mdia);
        }

        mp4EndAtom(b, trak);
    }

    mp4EndAtom(b, moov);
    return b;
}
} // namespace

// Apply white balance to raw bayer data
void applyWhiteBalance(const uint16_t* input, uint16_t* output,
                       int width, int height,
                       float rGain, float gGain, float bGain,
                       BayerPattern pattern,
                       const std::vector<uint16_t>& blackLevel)
{
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            int idx = bayerIndex(x, y, width);
            int color = bayerColor(pattern, x, y);
            uint16_t black = blackLevel[color];

            float val = static_cast<float>(input[idx]) - static_cast<float>(black);

            switch(color) {
                case 0: val *= rGain; break; // R
                case 1: val *= gGain; break; // G
                case 2: val *= bGain; break; // B
            }

            output[idx] = static_cast<uint16_t>(std::max(0.0f, val));
        }
    }
}

// Simple bilinear demosaicing - converts Bayer to RGB
void demosaicBilinear(const uint16_t* bayer, uint16_t* rgb,
                      int width, int height,
                      BayerPattern pattern)
{
    std::vector<uint16_t> r(width * height);
    std::vector<uint16_t> g(width * height);
    std::vector<uint16_t> b(width * height);

    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            int idx = bayerIndex(x, y, width);
            int color = bayerColor(pattern, x, y);

            // Collect neighbors
            std::vector<uint16_t> rNeighbors, gNeighbors, bNeighbors;

            for (int dy = -1; dy <= 1; dy++) {
                for (int dx = -1; dx <= 1; dx++) {
                    int nx = x + dx;
                    int ny = y + dy;

                    if (nx >= 0 && nx < width && ny >= 0 && ny < height) {
                        int nidx = bayerIndex(nx, ny, width);
                        int ncolor = bayerColor(pattern, nx, ny);

                        if (ncolor == 0) rNeighbors.push_back(bayer[nidx]);
                        else if (ncolor == 1) gNeighbors.push_back(bayer[nidx]);
                        else if (ncolor == 2) bNeighbors.push_back(bayer[nidx]);
                    }
                }
            }

            // Current pixel value
            if (color == 0) rNeighbors.push_back(bayer[idx]);
            else if (color == 1) gNeighbors.push_back(bayer[idx]);
            else if (color == 2) bNeighbors.push_back(bayer[idx]);

            // Average each channel
            uint16_t rVal = 0, gVal = 0, bVal = 0;

            if (!rNeighbors.empty()) {
                uint32_t sum = 0;
                for (auto v : rNeighbors) sum += v;
                rVal = sum / rNeighbors.size();
            }

            if (!gNeighbors.empty()) {
                uint32_t sum = 0;
                for (auto v : gNeighbors) sum += v;
                gVal = sum / gNeighbors.size();
            }

            if (!bNeighbors.empty()) {
                uint32_t sum = 0;
                for (auto v : bNeighbors) sum += v;
                bVal = sum / bNeighbors.size();
            }

            rgb[idx * 3 + 0] = rVal;
            rgb[idx * 3 + 1] = gVal;
            rgb[idx * 3 + 2] = bVal;
        }
    }
}

// Convert RGB16 to YUV 4:2:2 10-bit (v210 format)
// v210 packs 4 YUV samples in 5 32-bit words:
//   U0 Y0 Y1 V0 Y2 U1 Y3 Y4 V1 Y5 U2 Y6 Y7 V2
// Each 10-bit component
void rgb16ToYuv422_10bit(const uint16_t* rgb, uint32_t* yuv10,
                         int width, int height,
                         int pitchYuv)
{
    // Rec. 709 coefficients
    const float kr = 0.2126f;
    const float kg = 0.7152f;
    const float kb = 0.0722f;

    // Scale from 16-bit to 10-bit (assuming input is ~10-12 bits stored in 16-bit)
    const float scale = 1023.0f / 65535.0f;

    for (int y = 0; y < height; y++) {
        const uint16_t* rgbRow = rgb + y * width * 3;
        uint32_t* yuvRow = (uint32_t*)((uint8_t*)yuv10 + y * pitchYuv);

        int x = 0;
        int outIdx = 0;

        // Process 6 pixels at a time (4 YUV samples packed)
        while (x + 5 < width) {
            uint16_t yVals[6], uVals[3], vVals[3];

            for (int i = 0; i < 6; i += 2) {
                uint16_t r0 = rgbRow[(x + i) * 3 + 0];
                uint16_t g0 = rgbRow[(x + i) * 3 + 1];
                uint16_t b0 = rgbRow[(x + i) * 3 + 2];

                uint16_t r1 = rgbRow[(x + i + 1) * 3 + 0];
                uint16_t g1 = rgbRow[(x + i + 1) * 3 + 1];
                uint16_t b1 = rgbRow[(x + i + 1) * 3 + 2];

                float r = (r0 + r1) * 0.5f * scale;
                float g = (g0 + g1) * 0.5f * scale;
                float b = (b0 + b1) * 0.5f * scale;

                float yVal = kr * r + kg * g + kb * b;
                float uVal = 0.5f * (b - yVal) / (1.0f - kb) + 0.5f;
                float vVal = 0.5f * (r - yVal) / (1.0f - kr) + 0.5f;

                yVals[i] = static_cast<uint16_t>(yVal * 1023.0f);
                yVals[i + 1] = static_cast<uint16_t>(yVal * 1023.0f);
                uVals[i / 2] = static_cast<uint16_t>(std::max(0.0f, std::min(1.0f, uVal)) * 1023.0f);
                vVals[i / 2] = static_cast<uint16_t>(std::max(0.0f, std::min(1.0f, vVal)) * 1023.0f);
            }

            // Pack as v210: U0 Y0 Y1 V0 Y2 U1 Y3 Y4 V1 Y5 U2 Y6 Y7 V2 (for 8 pixels)
            // For 6 pixels we pack into 4 32-bit words:
            // Word0: U0(2) Y0(0) Y0(1) V0(2) Y1(0) Y1(1) U1(2)
            // Actually v210 format: (from low to high bits in each word)
            // word0: [U0(9:0)] [Y0(9:0)] (shifted)
            // Proper v210 packing (4 pixels in 5 words, but simplified here)

            // Simplified packing for 4:2:2 YCbCr 10-bit
            // Each pair of pixels: Y0 U Y1 V
            // Using standard YUY2-like packing but 10-bit

            // Write 4 words for 6 pixels
            // Word 0: U0 bits 9-0 (low 10) + Y0 bits 9-0 (high 10) - partially packed
            // For simplicity, using a simpler layout

            // Proper v210 packing:
            // cl0 = (U0<<20) | (Y0<<10) | V0
            // cl1 = (Y2<<20) | (U1<<10) | Y1
            // cl2 = (V1<<20) | (Y3<<10) | Y4
            // cl3 = (U2<<20) | (Y5<<10) | V2
            // (for 6 pixels: U0,Y0,V0,Y1,U1,Y2,V1,Y3,U2,Y4,V2,Y5)

            yuvRow[outIdx++] = (static_cast<uint32_t>(uVals[0]) << 20) |
                               (static_cast<uint32_t>(yVals[0]) << 10) |
                               vVals[0];
            yuvRow[outIdx++] = (static_cast<uint32_t>(yVals[2]) << 20) |
                               (static_cast<uint32_t>(uVals[1]) << 10) |
                               yVals[1];
            yuvRow[outIdx++] = (static_cast<uint32_t>(vVals[1]) << 20) |
                               (static_cast<uint32_t>(yVals[3]) << 10) |
                               yVals[4];
            yuvRow[outIdx++] = (static_cast<uint32_t>(uVals[2]) << 20) |
                               (static_cast<uint32_t>(yVals[5]) << 10) |
                               vVals[2];

            x += 6;
        }

        // Handle remaining pixels
        while (x < width) {
            uint16_t r = rgbRow[x * 3 + 0];
            uint16_t g = rgbRow[x * 3 + 1];
            uint16_t b = rgbRow[x * 3 + 2];

            float rf = r * scale, gf = g * scale, bf = b * scale;
            float yVal = kr * rf + kg * gf + kb * bf;
            float uVal = 0.5f * (bf - yVal) / (1.0f - kb) + 0.5f;
            float vVal = 0.5f * (rf - yVal) / (1.0f - kr) + 0.5f;

            // Simple padding for odd pixels
            uint16_t y10 = static_cast<uint16_t>(yVal * 1023.0f);
            uint16_t u10 = static_cast<uint16_t>(std::max(0.0f, std::min(1.0f, uVal)) * 1023.0f);
            uint16_t v10 = static_cast<uint16_t>(std::max(0.0f, std::min(1.0f, vVal)) * 1023.0f);

            // Pack last pixels
            if (x + 1 < width) {
                uint16_t r2 = rgbRow[(x+1) * 3 + 0];
                uint16_t g2 = rgbRow[(x+1) * 3 + 1];
                uint16_t b2 = rgbRow[(x+1) * 3 + 2];

                float rf2 = r2 * scale, gf2 = g2 * scale, bf2 = b2 * scale;
                float yVal2 = kr * rf2 + kg * gf2 + kb * bf2;

                uint16_t y210 = static_cast<uint16_t>(yVal2 * 1023.0f);
                yuvRow[outIdx++] = (static_cast<uint32_t>(u10) << 20) |
                                   (static_cast<uint32_t>(y10) << 10) |
                                   v10;
                yuvRow[outIdx++] = (static_cast<uint32_t>(y210) << 20) |
                                   (static_cast<uint32_t>(u10) << 10) |
                                   y10;
                x += 2;
            } else {
                yuvRow[outIdx++] = (static_cast<uint32_t>(u10) << 20) |
                                   (static_cast<uint32_t>(y10) << 10) |
                                   v10;
                x++;
            }
        }
    }
}

// Encode frame to CineForm
CFHD_Error encodeToCineform(CFHD_EncoderRef encoderRef,
                            const uint16_t* rawBayer,
                            int width, int height,
                            const std::vector<float>& asShotNeutral,
                            const std::vector<uint16_t>& blackLevel,
                            const std::string& sensorArrangement,
                            std::vector<uint8_t>& encodedOutput)
{
    // Determine bayer pattern
    BayerPattern pattern;
    if (sensorArrangement == "rggb") pattern = RGGB;
    else if (sensorArrangement == "bggr") pattern = BGGR;
    else if (sensorArrangement == "grbg") pattern = GRBG;
    else if (sensorArrangement == "gbrg") pattern = GBRG;
    else return CFHD_ERROR_BADFORMAT;

    // Calculate white balance gains from asShotNeutral (R,G,B multipliers)
    // asShotNeutral values are normalized to G=1, so we compute gains
    float rGain = 1.0f, gGain = 1.0f, bGain = 1.0f;
    if (asShotNeutral.size() >= 3) {
        // asShotNeutral[0] is R gain, [1] is G gain, [2] is B gain
        // These are typically < 1.0 for channels that need gain
        // We invert to get multiplication factors
        rGain = (asShotNeutral[0] > 0) ? (1.0f / asShotNeutral[0]) : 1.0f;
        gGain = (asShotNeutral[1] > 0) ? (1.0f / asShotNeutral[1]) : 1.0f;
        bGain = (asShotNeutral[2] > 0) ? (1.0f / asShotNeutral[2]) : 1.0f;
    }

    // Apply white balance
    std::vector<uint16_t> wbBayer(width * height);
    applyWhiteBalance(rawBayer, wbBayer.data(), width, height,
                      rGain, gGain, bGain, pattern, blackLevel);

    // Demosaic to RGB
    std::vector<uint16_t> rgb(width * height * 3);
    demosaicBilinear(wbBayer.data(), rgb.data(), width, height, pattern);

    // Convert to YUV 4:2:2 10-bit (v210 format)
    // v210 pitch is rounded up to multiple of 48 bytes (16 pixels)
    int yuvPitch = ((width + 47) / 48) * 48 * 4;  // 4 bytes per 32-bit word
    std::vector<uint8_t> yuvBuffer(yuvPitch * height);

    rgb16ToYuv422_10bit(rgb.data(), (uint32_t*)yuvBuffer.data(),
                        width, height, yuvPitch);

    // Encode with CineForm
    CFHD_Error err = CFHD_EncodeSample(encoderRef, yuvBuffer.data(), yuvPitch);
    if (err != CFHD_ERROR_OKAY) return err;

    // Get encoded data
    void* sampleData = nullptr;
    size_t sampleSize = 0;
    err = CFHD_GetSampleData(encoderRef, &sampleData, &sampleSize);
    if (err != CFHD_ERROR_OKAY) return err;

    encodedOutput.assign(static_cast<uint8_t*>(sampleData),
                         static_cast<uint8_t*>(sampleData) + sampleSize);

    return CFHD_ERROR_OKAY;
}

void writeAudio(
    const std::string& outputPath,
    const int sampleRateHz,
    const int numChannels,
    std::vector<motioncam::AudioChunk>& audioChunks)
{
    AudioFile<int16_t> audio;
    
    audio.setNumChannels(numChannels);
    audio.setSampleRate(sampleRateHz);
    
    if(numChannels == 2) {
        for(auto& x : audioChunks) {
            for(auto i = 0; i < x.second.size(); i+=2) {
                audio.samples[0].push_back(x.second[i]);
                audio.samples[1].push_back(x.second[i+1]);
            }
        }
    }
    else if(numChannels == 1) {
        for(auto& x : audioChunks) {
            for(auto i = 0; i < x.second.size(); i++)
                audio.samples[0].push_back(x.second[i]);
        }
    }
    
    audio.save(outputPath);
}

void writeDng(
    const std::string& outputPath,
    const std::vector<uint8_t>& data,
    const nlohmann::json& metadata,
    const nlohmann::json& containerMetadata)
{
    const unsigned int width = metadata["width"];
    const unsigned int height = metadata["height"];
    
    std::vector<float> asShotNeutral = metadata["asShotNeutral"];

    std::vector<uint16_t> blackLevel = containerMetadata["blackLevel"];
    double whiteLevel = containerMetadata["whiteLevel"];
    std::string sensorArrangement = containerMetadata["sensorArrangment"];
    std::vector<float> colorMatrix1 = containerMetadata["colorMatrix1"];
    std::vector<float> colorMatrix2 = containerMetadata["colorMatrix2"];
    std::vector<float> forwardMatrix1 = containerMetadata["forwardMatrix1"];
    std::vector<float> forwardMatrix2 = containerMetadata["forwardMatrix2"];

    // Create first frame
    tinydngwriter::DNGImage dng;

    dng.SetBigEndian(false);
    dng.SetDNGVersion(1, 4, 0, 0);
    dng.SetDNGBackwardVersion(1, 1, 0, 0);
    dng.SetImageData(reinterpret_cast<const unsigned char*>(data.data()), data.size());
    dng.SetImageWidth(width);
    dng.SetImageLength(height);
    dng.SetPlanarConfig(tinydngwriter::PLANARCONFIG_CONTIG);
    dng.SetPhotometric(tinydngwriter::PHOTOMETRIC_CFA);
    dng.SetRowsPerStrip(height);
    dng.SetSamplesPerPixel(1);
    dng.SetCFARepeatPatternDim(2, 2);
    
    dng.SetBlackLevelRepeatDim(2, 2);
    dng.SetBlackLevel(4, blackLevel.data());
    dng.SetWhiteLevel(whiteLevel);
    dng.SetCompression(tinydngwriter::COMPRESSION_NONE);

    std::vector<uint8_t> cfa;
    
    if(sensorArrangement == "rggb")
        cfa = { 0, 1, 1, 2 };
    else if(sensorArrangement == "bggr")
        cfa = { 2, 1, 1, 0 };
    else if(sensorArrangement == "grbg")
        cfa = { 1, 0, 2, 1 };
    else if(sensorArrangement == "gbrg")
        cfa = { 1, 2, 0, 1 };
    else
        throw std::runtime_error("Invalid sensor arrangement");

    dng.SetCFAPattern(4, cfa.data());
    
    // Rectangular
    dng.SetCFALayout(1);

    const uint16_t bps[1] = { 16 };
    dng.SetBitsPerSample(1, bps);
    
    dng.SetColorMatrix1(3, colorMatrix1.data());
    dng.SetColorMatrix2(3, colorMatrix2.data());

    dng.SetForwardMatrix1(3, forwardMatrix1.data());
    dng.SetForwardMatrix2(3, forwardMatrix2.data());
    
    dng.SetAsShotNeutral(3, asShotNeutral.data());
    
    dng.SetCalibrationIlluminant1(21);
    dng.SetCalibrationIlluminant2(17);
    
    dng.SetUniqueCameraModel("MotionCam");
    dng.SetSubfileType();
    
    const uint32_t activeArea[4] = { 0, 0, height, width };
    dng.SetActiveArea(&activeArea[0]);

    // Write DNG
    std::string err;
    tinydngwriter::DNGWriter writer(false);

    writer.AddImage(&dng);

    writer.WriteToFile(outputPath.c_str(), &err);
}

int main(int argc, const char * argv[]) {
    if(argc < 2) {
        std::cout << "Usage: decoder <input file> [-n number of frames to export] [-c output.mp4]" << std::endl;
        std::cout << "  -n: number of frames to export (default: all)" << std::endl;
        std::cout << "  -c: encode to CineForm mp4 (output filename)" << std::endl;
        return -1;
    }

    std::string inputPath(argv[1]);
    int endFrame = -1;
    std::string cineformOutput;
    bool encodeCineform = false;

    // Parse arguments
    for(int i = 2; i < argc; i++) {
        if(std::string(argv[i]) == "-n" && i + 1 < argc) {
            endFrame = std::stoi(argv[++i]);
        }
        else if(std::string(argv[i]) == "-c" && i + 1 < argc) {
            cineformOutput = argv[++i];
            encodeCineform = true;
        }
    }

    // Open decoder
    CFHD_EncoderRef encoderRef = nullptr;
    FILE* mp4File = nullptr;
    std::vector<uint32_t> mp4SampleSizes;
    std::vector<uint32_t> mp4SampleOffsets;
    long mp4MdatSizePos = 0;
    uint32_t mp4Timescale = 30000;
    double mp4Fps = 30.0;
    uint32_t mp4Width = 0;
    uint32_t mp4Height = 0;

    try {
        motioncam::Decoder d(inputPath);

        auto frames = d.getFrames();
        auto containerMetadata = d.getContainerMetadata();
        char path[32];

        std::cout << "Found " << frames.size() << " frames" << std::endl;

        if(endFrame < 0)
            endFrame = static_cast<int>(frames.size());

        endFrame = std::min(static_cast<int>(frames.size()), std::max(0, endFrame));

        // Get metadata needed for encoding
        std::vector<uint16_t> blackLevel = containerMetadata["blackLevel"];
        std::string sensorArrangement = containerMetadata["sensorArrangment"];

        // Initialize CineForm encoder if requested
        if(encodeCineform) {
            // Load first frame to get dimensions
            std::vector<uint8_t> data;
            nlohmann::json metadata;
            d.loadFrame(frames[0], data, metadata);

            int width = metadata["width"];
            int height = metadata["height"];
            mp4Width = static_cast<uint32_t>(width);
            mp4Height = static_cast<uint32_t>(height);

            std::cout << "Encoding to CineForm " << width << "x" << height << " 4:2:2 10-bit" << std::endl;

            // Open encoder
            CFHD_Error err = CFHD_OpenEncoder(&encoderRef, nullptr);
            if(err != CFHD_ERROR_OKAY) {
                std::cerr << "Failed to open CineForm encoder: " << err << std::endl;
                return -1;
            }

            // Prepare encoder for v210 format (10-bit 4:2:2 YUV)
            err = CFHD_PrepareToEncode(encoderRef,
                                       width,
                                       height,
                                       CFHD_PIXEL_FORMAT_V210,
                                       CFHD_ENCODED_FORMAT_YUV_422,
                                       CFHD_ENCODING_FLAGS_NONE,
                                       CFHD_ENCODING_QUALITY_FILMSCAN1);
            if(err != CFHD_ERROR_OKAY) {
                std::cerr << "Failed to prepare encoder: " << err << std::endl;
                CFHD_CloseEncoder(encoderRef);
                return -1;
            }

            // Create output file and write minimal MP4 header
            mp4File = std::fopen(cineformOutput.c_str(), "wb");
            if(!mp4File) {
                std::cerr << "Failed to create output file: " << cineformOutput << std::endl;
                CFHD_CloseEncoder(encoderRef);
                return -1;
            }

            // ftyp
            {
                uint32_t ftypSize = 24;
                uint8_t ftyp[24] = {
                    0x00,0x00,0x00,0x18,'f','t','y','p',
                    'q','t',' ',' ',0x00,0x00,0x02,0x00,
                    'q','t',' ',' ','i','s','o','m'
                };
                std::fwrite(ftyp, 1, ftypSize, mp4File);
            }

            // mdat header (size patched later)
            mp4MdatSizePos = std::ftell(mp4File);
            uint8_t mdatHeader[8] = {0,0,0,0,'m','d','a','t'};
            std::fwrite(mdatHeader, 1, 8, mp4File);
        }

        // Write audio
        std::vector<motioncam::AudioChunk> audioChunks;
        d.loadAudio(audioChunks);
        writeAudio("audio.wav", d.audioSampleRateHz(), d.numAudioChannels(), audioChunks);

        // Process frames
        std::vector<uint8_t> data;
        nlohmann::json metadata;

        for(int i = 0; i < endFrame; i++) {
            d.loadFrame(frames[i], data, metadata);

            int width = metadata["width"];
            int height = metadata["height"];
            std::vector<float> asShotNeutral = metadata["asShotNeutral"];
            if (i == 0 && metadata.contains("frameRate") && metadata["frameRate"].is_number()) {
                mp4Fps = metadata["frameRate"].get<double>();
            }

            if(encodeCineform) {
                // Encode to CineForm
                std::vector<uint8_t> encodedFrame;
                CFHD_Error err = encodeToCineform(encoderRef,
                                                   reinterpret_cast<const uint16_t*>(data.data()),
                                                   width, height,
                                                   asShotNeutral,
                                                   blackLevel,
                                                   sensorArrangement,
                                                   encodedFrame);

                if(err != CFHD_ERROR_OKAY) {
                    std::cerr << "Error encoding frame " << i << ": " << err << std::endl;
                    break;
                }

                // Write encoded frame to file (MP4 mdat)
                long sampleOffset = std::ftell(mp4File);
                if (sampleOffset < 0) {
                    std::cerr << "Failed to get file position" << std::endl;
                    break;
                }
                if (sampleOffset > 0xffffffffL) {
                    std::cerr << "MP4 sample offset exceeds 32-bit stco limit" << std::endl;
                    break;
                }
                mp4SampleOffsets.push_back(static_cast<uint32_t>(sampleOffset));
                mp4SampleSizes.push_back(static_cast<uint32_t>(encodedFrame.size()));
                std::fwrite(encodedFrame.data(), 1, encodedFrame.size(), mp4File);

                std::cout << "Encoded frame " << (i+1) << "/" << endFrame
                         << " (" << encodedFrame.size() << " bytes)" << std::endl;
            }
            else {
                // Write DNG
                std::snprintf(path, sizeof(path), "frame_%06d.dng", i);
                std::cout << "Writing " << path << std::endl;
                writeDng(std::string(path), data, metadata, containerMetadata);
            }
        }

        // Finalize MP4 (patch mdat size, write moov)
        if (encodeCineform && mp4File) {
            long mdatEnd = std::ftell(mp4File);
            if (mdatEnd >= 0) {
                uint32_t mdatSize = static_cast<uint32_t>(mdatEnd - mp4MdatSizePos);
                std::fseek(mp4File, mp4MdatSizePos, SEEK_SET);
                uint8_t sizeBuf[4] = {
                    static_cast<uint8_t>((mdatSize >> 24) & 0xff),
                    static_cast<uint8_t>((mdatSize >> 16) & 0xff),
                    static_cast<uint8_t>((mdatSize >> 8) & 0xff),
                    static_cast<uint8_t>(mdatSize & 0xff)
                };
                std::fwrite(sizeBuf, 1, 4, mp4File);

                // Append moov
                std::fseek(mp4File, mdatEnd, SEEK_SET);
                uint32_t frameCount = static_cast<uint32_t>(mp4SampleSizes.size());
                if (frameCount > 0) {
                    if (mp4Fps <= 0.0) mp4Fps = 30.0;
                    uint32_t sampleDuration = static_cast<uint32_t>(std::llround(mp4Timescale / mp4Fps));
                    if (sampleDuration == 0) sampleDuration = 1;
                    uint32_t duration = sampleDuration * frameCount;
                    Mp4Buffer moov = buildMoov(mp4Timescale, duration, mp4Width,
                                               mp4Height, mp4SampleSizes, mp4SampleOffsets);
                    std::fwrite(moov.data.data(), 1, moov.data.size(), mp4File);
                }
            }
        }

        // Cleanup encoder
        if(encoderRef) {
            CFHD_CloseEncoder(encoderRef);
            encoderRef = nullptr;
        }
        if(mp4File) {
            std::fclose(mp4File);
            mp4File = nullptr;
        }

        std::cout << "Done!" << std::endl;
    }
    catch(motioncam::MotionCamException& e) {
        std::cerr << "Error: " << e.what() << std::endl;

        if(encoderRef) CFHD_CloseEncoder(encoderRef);
        if(mp4File) std::fclose(mp4File);

        return -1;
    }

    return 0;
}
