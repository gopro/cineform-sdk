#include <iostream>
#include <vector>
#include <cstdint>
#include <cstdio>
#include <cmath>
#include <algorithm>

#include <motioncam/Decoder.hpp>

#include <CFHDEncoder.h>
#include <CFHDMetadata.h>
#include <CFHDTypes.h>
#include <CFHDDecoder.h>

enum BayerPattern { RGGB = 0, BGGR = 1, GRBG = 2, GBRG = 3 };

static inline int bayerIndex(int x, int y, int width) { return y * width + x; }

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

namespace {
struct Mp4Buffer { std::vector<uint8_t> data; };

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
static inline void mp4WriteFourCC(Mp4Buffer& b, const char cc[4]) {
    mp4WriteU8(b, static_cast<uint8_t>(cc[0]));
    mp4WriteU8(b, static_cast<uint8_t>(cc[1]));
    mp4WriteU8(b, static_cast<uint8_t>(cc[2]));
    mp4WriteU8(b, static_cast<uint8_t>(cc[3]));
}
static inline size_t mp4StartAtom(Mp4Buffer& b, const char cc[4]) {
    size_t start = b.data.size();
    mp4WriteU32(b, 0);
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
                           uint32_t sampleDuration,
                           uint32_t width,
                           uint32_t height,
                           const std::vector<uint32_t>& sampleSizes,
                           const std::vector<uint32_t>& sampleOffsets)
{
    Mp4Buffer b;

    size_t moov = mp4StartAtom(b, "moov");
    {
        size_t mvhd = mp4StartAtom(b, "mvhd");
        mp4WriteU32(b, 0);
        mp4WriteU32(b, 0);
        mp4WriteU32(b, 0);
        mp4WriteU32(b, timescale);
        mp4WriteU32(b, duration);
        mp4WriteU32(b, 0x00010000);
        mp4WriteU16(b, 0x0100);
        mp4WriteU16(b, 0);
        mp4WriteU32(b, 0);
        mp4WriteU32(b, 0);
        mp4WriteU32(b, 0x00010000);
        mp4WriteU32(b, 0);
        mp4WriteU32(b, 0);
        mp4WriteU32(b, 0);
        mp4WriteU32(b, 0x00010000);
        mp4WriteU32(b, 0);
        mp4WriteU32(b, 0);
        mp4WriteU32(b, 0);
        mp4WriteU32(b, 0x40000000);
        for (int i = 0; i < 6; i++) mp4WriteU32(b, 0);
        mp4WriteU32(b, 1);
        mp4EndAtom(b, mvhd);
    }
    {
        size_t trak = mp4StartAtom(b, "trak");
        {
            size_t tkhd = mp4StartAtom(b, "tkhd");
            mp4WriteU32(b, 0x00000007);
            mp4WriteU32(b, 0);
            mp4WriteU32(b, 0);
            mp4WriteU32(b, 1);
            mp4WriteU32(b, 0);
            mp4WriteU32(b, duration);
            mp4WriteU32(b, 0);
            mp4WriteU32(b, 0);
            mp4WriteU16(b, 0);
            mp4WriteU16(b, 0);
            mp4WriteU16(b, 0);
            mp4WriteU16(b, 0);
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
        {
            size_t mdia = mp4StartAtom(b, "mdia");
            {
                size_t mdhd = mp4StartAtom(b, "mdhd");
                mp4WriteU32(b, 0);
                mp4WriteU32(b, 0);
                mp4WriteU32(b, 0);
                mp4WriteU32(b, timescale);
                mp4WriteU32(b, duration);
                mp4WriteU16(b, 0x55c4);
                mp4WriteU16(b, 0);
                mp4EndAtom(b, mdhd);
            }
            {
                size_t hdlr = mp4StartAtom(b, "hdlr");
                mp4WriteU32(b, 0);
                mp4WriteU32(b, 0);
                mp4WriteFourCC(b, "vide");
                mp4WriteU32(b, 0);
                mp4WriteU32(b, 0);
                mp4WriteU32(b, 0);
                const char name[] = "VideoHandler";
                for (size_t i = 0; i < sizeof(name); i++) mp4WriteU8(b, static_cast<uint8_t>(name[i]));
                mp4EndAtom(b, hdlr);
            }
            {
                size_t minf = mp4StartAtom(b, "minf");
                {
                    size_t vmhd = mp4StartAtom(b, "vmhd");
                    mp4WriteU32(b, 0x00000001);
                    mp4WriteU16(b, 0);
                    mp4WriteU16(b, 0);
                    mp4WriteU16(b, 0);
                    mp4WriteU16(b, 0);
                    mp4EndAtom(b, vmhd);
                }
                {
                    size_t dinf = mp4StartAtom(b, "dinf");
                    size_t dref = mp4StartAtom(b, "dref");
                    mp4WriteU32(b, 0);
                    mp4WriteU32(b, 1);
                    size_t url = mp4StartAtom(b, "url ");
                    mp4WriteU32(b, 0x00000001);
                    mp4EndAtom(b, url);
                    mp4EndAtom(b, dref);
                    mp4EndAtom(b, dinf);
                }
                {
                    size_t stbl = mp4StartAtom(b, "stbl");
                    {
                        size_t stsd = mp4StartAtom(b, "stsd");
                        mp4WriteU32(b, 0);
                        mp4WriteU32(b, 1);

                        size_t cfhd = mp4StartAtom(b, "CFHD");
                        for (int i = 0; i < 6; i++) mp4WriteU8(b, 0);
                        mp4WriteU16(b, 1);
                        mp4WriteU16(b, 0);
                        mp4WriteU16(b, 0);
                        mp4WriteU32(b, 0);
                        mp4WriteU32(b, 0);
                        mp4WriteU32(b, 0);
                        mp4WriteU16(b, static_cast<uint16_t>(width));
                        mp4WriteU16(b, static_cast<uint16_t>(height));
                        mp4WriteU32(b, 0x00480000);
                        mp4WriteU32(b, 0x00480000);
                        mp4WriteU32(b, 0);
                        mp4WriteU16(b, 1);
                        mp4WriteU8(b, 0);
                        for (int i = 0; i < 31; i++) mp4WriteU8(b, 0);
                        mp4WriteU16(b, 0x0018);
                        mp4WriteU16(b, 0xffff);
                        mp4EndAtom(b, cfhd);

                        mp4EndAtom(b, stsd);
                    }
                    {
                        size_t stts = mp4StartAtom(b, "stts");
                        mp4WriteU32(b, 0);
                        mp4WriteU32(b, 1);
                        mp4WriteU32(b, static_cast<uint32_t>(sampleSizes.size()));
                        mp4WriteU32(b, sampleDuration);
                        mp4EndAtom(b, stts);
                    }
                    {
                        size_t stsc = mp4StartAtom(b, "stsc");
                        mp4WriteU32(b, 0);
                        mp4WriteU32(b, 1);
                        mp4WriteU32(b, 1);
                        mp4WriteU32(b, 1);
                        mp4WriteU32(b, 1);
                        mp4EndAtom(b, stsc);
                    }
                    {
                        size_t stsz = mp4StartAtom(b, "stsz");
                        mp4WriteU32(b, 0);
                        mp4WriteU32(b, 0);
                        mp4WriteU32(b, static_cast<uint32_t>(sampleSizes.size()));
                        for (uint32_t s : sampleSizes) mp4WriteU32(b, s);
                        mp4EndAtom(b, stsz);
                    }
                    {
                        size_t stco = mp4StartAtom(b, "stco");
                        mp4WriteU32(b, 0);
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
}

static inline uint16_t clamp10(int v) {
    if (v < 0) return 0;
    if (v > 1023) return 1023;
    return static_cast<uint16_t>(v);
}

static inline uint16_t clamp10u(uint32_t v) {
    return static_cast<uint16_t>(v > 1023u ? 1023u : v);
}

void demosaicBilinear10(const uint16_t* bayer, uint16_t* rgb, int width, int height, BayerPattern pattern)
{
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            int idx = bayerIndex(x, y, width);
            int color = bayerColor(pattern, x, y);

            uint32_t rSum = 0, gSum = 0, bSum = 0;
            uint32_t rCnt = 0, gCnt = 0, bCnt = 0;

            for (int dy = -1; dy <= 1; dy++) {
                for (int dx = -1; dx <= 1; dx++) {
                    int nx = x + dx;
                    int ny = y + dy;
                    if (nx < 0 || nx >= width || ny < 0 || ny >= height) continue;

                    int nidx = bayerIndex(nx, ny, width);
                    int ncolor = bayerColor(pattern, nx, ny);
                    uint16_t v = clamp10u(bayer[nidx]);

                    if (ncolor == 0) { rSum += v; rCnt++; }
                    else if (ncolor == 1) { gSum += v; gCnt++; }
                    else { bSum += v; bCnt++; }
                }
            }

            uint16_t r = rCnt ? static_cast<uint16_t>(rSum / rCnt) : 0;
            uint16_t g = gCnt ? static_cast<uint16_t>(gSum / gCnt) : 0;
            uint16_t b = bCnt ? static_cast<uint16_t>(bSum / bCnt) : 0;

            uint16_t center = clamp10u(bayer[idx]);
            if (color == 0) r = center;
            else if (color == 1) g = center;
            else b = center;

            rgb[idx * 3 + 0] = clamp10(r);
            rgb[idx * 3 + 1] = clamp10(g);
            rgb[idx * 3 + 2] = clamp10(b);
        }
    }
}

void rgb10ToV210(const uint16_t* rgb, uint32_t* v210, int width, int height, int pitchBytes)
{
    const float kr = 0.2126f;
    const float kg = 0.7152f;
    const float kb = 0.0722f;

    const float invMax = 1.0f / 1023.0f;
    const float yScale = 876.0f;
    const float cScale = 448.0f;

    int rowWords = pitchBytes / 4;

    for (int y = 0; y < height; y++) {
        const uint16_t* rgbRow = rgb + y * width * 3;
        uint32_t* outRow = reinterpret_cast<uint32_t*>(
            reinterpret_cast<uint8_t*>(v210) + y * pitchBytes);

        int groups = (width + 5) / 6;

        for (int g = 0; g < groups; g++) {
            int x = g * 6;

            auto sample = [&](int px, int ch) -> float {
                int p = (px < width) ? px : (width - 1);
                return clamp10u(rgbRow[p * 3 + ch]) * invMax;
            };

            float r0 = sample(x + 0, 0), g0 = sample(x + 0, 1), b0 = sample(x + 0, 2);
            float r1 = sample(x + 1, 0), g1 = sample(x + 1, 1), b1 = sample(x + 1, 2);
            float r2 = sample(x + 2, 0), g2 = sample(x + 2, 1), b2 = sample(x + 2, 2);
            float r3 = sample(x + 3, 0), g3 = sample(x + 3, 1), b3 = sample(x + 3, 2);
            float r4 = sample(x + 4, 0), g4 = sample(x + 4, 1), b4 = sample(x + 4, 2);
            float r5 = sample(x + 5, 0), g5 = sample(x + 5, 1), b5 = sample(x + 5, 2);

            auto luma = [&](float r, float g, float b) -> float {
                return kr * r + kg * g + kb * b;
            };

            float y0 = luma(r0, g0, b0);
            float y1 = luma(r1, g1, b1);
            float y2 = luma(r2, g2, b2);
            float y3 = luma(r3, g3, b3);
            float y4 = luma(r4, g4, b4);
            float y5 = luma(r5, g5, b5);

            auto chroma = [&](float rA, float gA, float bA, float rB, float gB, float bB,
                              float& cbOut, float& crOut) {
                float r = 0.5f * (rA + rB);
                float g = 0.5f * (gA + gB);
                float b = 0.5f * (bA + bB);
                float y = luma(r, g, b);
                cbOut = (b - y) / (2.0f * (1.0f - kb));
                crOut = (r - y) / (2.0f * (1.0f - kr));
            };

            float cb0, cr0, cb1, cr1, cb2, cr2;
            chroma(r0, g0, b0, r1, g1, b1, cb0, cr0);
            chroma(r2, g2, b2, r3, g3, b3, cb1, cr1);
            chroma(r4, g4, b4, r5, g5, b5, cb2, cr2);

            uint16_t y0v = clamp10(static_cast<int>(std::lround(y0 * yScale + 64.0f)));
            uint16_t y1v = clamp10(static_cast<int>(std::lround(y1 * yScale + 64.0f)));
            uint16_t y2v = clamp10(static_cast<int>(std::lround(y2 * yScale + 64.0f)));
            uint16_t y3v = clamp10(static_cast<int>(std::lround(y3 * yScale + 64.0f)));
            uint16_t y4v = clamp10(static_cast<int>(std::lround(y4 * yScale + 64.0f)));
            uint16_t y5v = clamp10(static_cast<int>(std::lround(y5 * yScale + 64.0f)));

            uint16_t cb0v = clamp10(static_cast<int>(std::lround(cb0 * cScale + 512.0f)));
            uint16_t cr0v = clamp10(static_cast<int>(std::lround(cr0 * cScale + 512.0f)));
            uint16_t cb1v = clamp10(static_cast<int>(std::lround(cb1 * cScale + 512.0f)));
            uint16_t cr1v = clamp10(static_cast<int>(std::lround(cr1 * cScale + 512.0f)));
            uint16_t cb2v = clamp10(static_cast<int>(std::lround(cb2 * cScale + 512.0f)));
            uint16_t cr2v = clamp10(static_cast<int>(std::lround(cr2 * cScale + 512.0f)));

            int outIdx = g * 4;
            if (outIdx + 3 >= rowWords) break;

            outRow[outIdx + 0] = (static_cast<uint32_t>(cb0v)) |
                                 (static_cast<uint32_t>(y0v) << 10) |
                                 (static_cast<uint32_t>(cr0v) << 20);
            outRow[outIdx + 1] = (static_cast<uint32_t>(y1v)) |
                                 (static_cast<uint32_t>(cb1v) << 10) |
                                 (static_cast<uint32_t>(y2v) << 20);
            outRow[outIdx + 2] = (static_cast<uint32_t>(cr1v)) |
                                 (static_cast<uint32_t>(y3v) << 10) |
                                 (static_cast<uint32_t>(cb2v) << 20);
            outRow[outIdx + 3] = (static_cast<uint32_t>(y4v)) |
                                 (static_cast<uint32_t>(cr2v) << 10) |
                                 (static_cast<uint32_t>(y5v) << 20);
        }
    }
}

CFHD_Error encodeToCineform(CFHD_EncoderRef encoderRef,
                            const uint16_t* rawBayer10in16,
                            int width, int height,
                            BayerPattern pattern,
                            std::vector<uint8_t>& encodedOutput)
{
    std::vector<uint16_t> rgb(static_cast<size_t>(width) * static_cast<size_t>(height) * 3);
    demosaicBilinear10(rawBayer10in16, rgb.data(), width, height, pattern);

    int32_t yuvPitch = 0;
    CFHD_Error pitchErr = CFHD_GetImagePitch(static_cast<uint32_t>(width), CFHD_PIXEL_FORMAT_V210, &yuvPitch);
    if (pitchErr != CFHD_ERROR_OKAY || yuvPitch <= 0) return CFHD_ERROR_BADFORMAT;

    std::vector<uint8_t> yuvBuffer(static_cast<size_t>(yuvPitch) * static_cast<size_t>(height), 0);
    rgb10ToV210(rgb.data(), reinterpret_cast<uint32_t*>(yuvBuffer.data()), width, height, yuvPitch);

    CFHD_Error err = CFHD_EncodeSample(encoderRef, yuvBuffer.data(), yuvPitch);
    if (err != CFHD_ERROR_OKAY) return err;

    void* sampleData = nullptr;
    size_t sampleSize = 0;
    err = CFHD_GetSampleData(encoderRef, &sampleData, &sampleSize);
    if (err != CFHD_ERROR_OKAY) return err;

    encodedOutput.assign(static_cast<uint8_t*>(sampleData), static_cast<uint8_t*>(sampleData) + sampleSize);
    return CFHD_ERROR_OKAY;
}

int main(int argc, const char* argv[])
{
    if (argc != 3) {
        std::cout << "Usage: decoder <input file> <output.mp4>\n";
        return 1;
    }

    std::string inputPath(argv[1]);
    std::string outputPath(argv[2]);

    CFHD_EncoderRef encoderRef = nullptr;
    FILE* mp4File = nullptr;

    std::vector<uint32_t> sampleSizes;
    std::vector<uint32_t> sampleOffsets;

    long mdatSizePos = 0;
    uint32_t timescale = 30000;
    double fps = 30.0;
    uint32_t w = 0, h = 0;

    try {
        motioncam::Decoder d(inputPath);
        auto frames = d.getFrames();
        auto containerMetadata = d.getContainerMetadata();

        if (frames.empty()) return 1;

        std::vector<uint8_t> data;
        nlohmann::json metadata;
        d.loadFrame(frames[0], data, metadata);

        int width = metadata["width"];
        int height = metadata["height"];
        w = static_cast<uint32_t>(width);
        h = static_cast<uint32_t>(height);

        if (metadata.contains("frameRate") && metadata["frameRate"].is_number()) {
            fps = metadata["frameRate"].get<double>();
            if (fps <= 0.0) fps = 30.0;
        }

        CFHD_Error err = CFHD_OpenEncoder(&encoderRef, nullptr);
        if (err != CFHD_ERROR_OKAY) return 1;

        err = CFHD_PrepareToEncode(encoderRef,
                                   width,
                                   height,
                                   CFHD_PIXEL_FORMAT_V210,
                                   CFHD_ENCODED_FORMAT_YUV_422,
                                   CFHD_ENCODING_FLAGS_NONE,
                                   CFHD_ENCODING_QUALITY_FILMSCAN1);
        if (err != CFHD_ERROR_OKAY) {
            CFHD_CloseEncoder(encoderRef);
            return 1;
        }

        mp4File = std::fopen(outputPath.c_str(), "wb");
        if (!mp4File) {
            CFHD_CloseEncoder(encoderRef);
            return 1;
        }

        uint8_t ftyp[24] = {
            0x00,0x00,0x00,0x18,'f','t','y','p',
            'i','s','o','m',0x00,0x00,0x02,0x00,
            'i','s','o','m','i','s','o','2'
        };
        std::fwrite(ftyp, 1, 24, mp4File);

        mdatSizePos = std::ftell(mp4File);
        uint8_t mdatHeader[8] = {0,0,0,0,'m','d','a','t'};
        std::fwrite(mdatHeader, 1, 8, mp4File);

        BayerPattern pattern = RGGB;

        for (size_t i = 0; i < frames.size(); i++) {
            d.loadFrame(frames[i], data, metadata);

            std::vector<uint8_t> encoded;
            err = encodeToCineform(encoderRef,
                                   reinterpret_cast<const uint16_t*>(data.data()),
                                   width, height,
                                   pattern,
                                   encoded);
            if (err != CFHD_ERROR_OKAY) break;

            long off = std::ftell(mp4File);
            if (off < 0 || off > 0xffffffffL) break;

            sampleOffsets.push_back(static_cast<uint32_t>(off));
            sampleSizes.push_back(static_cast<uint32_t>(encoded.size()));
            std::fwrite(encoded.data(), 1, encoded.size(), mp4File);
        }

        long mdatEnd = std::ftell(mp4File);
        if (mdatEnd >= 0) {
            uint32_t mdatSize = static_cast<uint32_t>(mdatEnd - mdatSizePos);
            std::fseek(mp4File, mdatSizePos, SEEK_SET);
            uint8_t sizeBuf[4] = {
                static_cast<uint8_t>((mdatSize >> 24) & 0xff),
                static_cast<uint8_t>((mdatSize >> 16) & 0xff),
                static_cast<uint8_t>((mdatSize >> 8) & 0xff),
                static_cast<uint8_t>(mdatSize & 0xff)
            };
            std::fwrite(sizeBuf, 1, 4, mp4File);
            std::fseek(mp4File, mdatEnd, SEEK_SET);

            uint32_t frameCount = static_cast<uint32_t>(sampleSizes.size());
            if (frameCount > 0) {
                uint32_t sampleDuration = static_cast<uint32_t>(std::llround(static_cast<double>(timescale) / fps));
                if (sampleDuration == 0) sampleDuration = 1;
                uint32_t duration = sampleDuration * frameCount;

                Mp4Buffer moov = buildMoov(timescale, duration, sampleDuration, w, h, sampleSizes, sampleOffsets);
                std::fwrite(moov.data.data(), 1, moov.data.size(), mp4File);
            }
        }

        if (encoderRef) CFHD_CloseEncoder(encoderRef);
        if (mp4File) std::fclose(mp4File);
    }
    catch(...) {
        if (encoderRef) CFHD_CloseEncoder(encoderRef);
        if (mp4File) std::fclose(mp4File);
        return 1;
    }

    return 0;
}
