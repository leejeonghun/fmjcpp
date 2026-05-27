#include "fli.h"
#include <cstdio>
#include <algorithm>

FliPlayer::FliPlayer(SDL_Renderer* renderer)
    : renderer(renderer),
    frameTexture(nullptr),
    framePixels(320 * 200, 0),
    frameBuffer32(320 * 200, 0),
    fliFile(nullptr),
    totalFrames(0),
    currentFrameIndex(0),
    lastFrameTimeMs(0)
{
    palette.fill(0);
}

FliPlayer::~FliPlayer() {
    if (frameTexture) {
        SDL_DestroyTexture(frameTexture);
    }
    if (fliFile) {
        fclose(fliFile);
    }
}

bool FliPlayer::load(const std::string& filename) {
    if (fliFile) {
        fclose(fliFile);
        fliFile = nullptr;
    }

    fliFile = fopen(filename.c_str(), "rb");
    if (!fliFile) return false;

    uint32_t size;
    uint16_t type;
    if (fread(&size, 4, 1, fliFile) != 1) return false;
    if (fread(&type, 2, 1, fliFile) != 1) return false;

    if (type != FLI_MAGIC_ID) {
        fclose(fliFile);
        fliFile = nullptr;
        return false;
    }

    if (fread(&totalFrames, 2, 1, fliFile) != 1) return false;
    fseek(fliFile, 128, SEEK_SET); // Skip the 128-byte Autodesk Animator file header

    currentFrameIndex = 0;
    return true;
}

bool FliPlayer::update() {
    if (!fliFile) return false;

    uint32_t currentTimeMs = SDL_GetTicks();
    if (currentTimeMs - lastFrameTimeMs < FRAME_DELAY_MS) {
        return true;
    }
    lastFrameTimeMs = currentTimeMs;

    if (currentFrameIndex >= totalFrames) {
        return false;
    }

    decodeNextFrame();
    currentFrameIndex++;

    updateTexture();

    return true;
}

void FliPlayer::decodeNextFrame() {
    long frameStart = ftell(fliFile);
    uint32_t frameSize;
    uint16_t frameType;
    uint16_t numChunks;

    if (fread(&frameSize, 4, 1, fliFile) != 1) return;
    if (fread(&frameType, 2, 1, fliFile) != 1) return;
    if (fread(&numChunks, 2, 1, fliFile) != 1) return;
    fseek(fliFile, 8, SEEK_CUR); // Skip reserved 8-byte padding

    for (int i = 0; i < numChunks; ++i) {
        long chunkStart = ftell(fliFile);
        uint32_t chunkSize;
        uint16_t chunkType;
        if (fread(&chunkSize, 4, 1, fliFile) != 1) return;
        if (fread(&chunkType, 2, 1, fliFile) != 1) return;

        FliChunkType type = static_cast<FliChunkType>(chunkType);

        if (type == FliChunkType::COLOR_64 || type == FliChunkType::COLOR_256) {
            uint16_t numPackets;
            if (fread(&numPackets, 2, 1, fliFile) != 1) return;
            int skip = 0;
            for (int p = 0; p < numPackets; ++p) {
                uint8_t colorsToSkip, colorsToChange;
                if (fread(&colorsToSkip, 1, 1, fliFile) != 1) return;
                if (fread(&colorsToChange, 1, 1, fliFile) != 1) return;
                skip += colorsToSkip;
                int count = (colorsToChange == 0) ? 256 : colorsToChange;
                for (int c = 0; c < count; ++c) {
                    uint8_t r, g, b;
                    if (fread(&r, 1, 1, fliFile) != 1) return;
                    if (fread(&g, 1, 1, fliFile) != 1) return;
                    if (fread(&b, 1, 1, fliFile) != 1) return;

                    // FLI palette contains 6-bit values (0-63). Translate to standard 8-bit scale
                    uint8_t red = static_cast<uint8_t>((r << 2) | (r >> 4));
                    uint8_t green = static_cast<uint8_t>((g << 2) | (g >> 4));
                    uint8_t blue = static_cast<uint8_t>((b << 2) | (b >> 4));

                    palette[skip + c] = (static_cast<uint32_t>(red) << 16) |
                        (static_cast<uint32_t>(green) << 8) |
                        static_cast<uint32_t>(blue);
                }
                skip += count;
            }
        }
        else if (type == FliChunkType::DELTA_LINES) {
            uint16_t startLine, numLines;
            if (fread(&startLine, 2, 1, fliFile) != 1) return;
            if (fread(&numLines, 2, 1, fliFile) != 1) return;
            for (int y = startLine; y < startLine + numLines; ++y) {
                uint8_t numPackets;
                if (fread(&numPackets, 1, 1, fliFile) != 1) return;
                int x = 0;
                for (int p = 0; p < numPackets; ++p) {
                    uint8_t skip;
                    int8_t count;
                    if (fread(&skip, 1, 1, fliFile) != 1) return;
                    if (fread(&count, 1, 1, fliFile) != 1) return;
                    x += skip;
                    if (count > 0) {
                        if (fread(&framePixels[y * 320 + x], count, 1, fliFile) != 1) return;
                        x += count;
                    }
                    else if (count < 0) {
                        count = -count;
                        uint8_t color;
                        if (fread(&color, 1, 1, fliFile) != 1) return;
                        std::fill(framePixels.begin() + (y * 320 + x), framePixels.begin() + (y * 320 + x + count), color);
                        x += count;
                    }
                }
            }
        }
        else if (type == FliChunkType::BLACK_FRAME) {
            std::fill(framePixels.begin(), framePixels.end(), 0);
        }
        else if (type == FliChunkType::BYTE_RUN) {
            for (int y = 0; y < 200; ++y) {
                uint8_t numPackets;
                if (fread(&numPackets, 1, 1, fliFile) != 1) return;
                int x = 0;
                for (int p = 0; p < numPackets; ++p) {
                    int8_t count;
                    if (fread(&count, 1, 1, fliFile) != 1) return;
                    if (count > 0) {
                        uint8_t color;
                        if (fread(&color, 1, 1, fliFile) != 1) return;
                        std::fill(framePixels.begin() + (y * 320 + x), framePixels.begin() + (y * 320 + x + count), color);
                        x += count;
                    }
                    else if (count < 0) {
                        count = -count;
                        if (fread(&framePixels[y * 320 + x], count, 1, fliFile) != 1) return;
                        x += count;
                    }
                }
            }
        }
        else if (type == FliChunkType::RAW_COPY) {
            if (fread(framePixels.data(), 320 * 200, 1, fliFile) != 1) return;
        }

        fseek(fliFile, chunkStart + chunkSize, SEEK_SET);
    }
    fseek(fliFile, frameStart + frameSize, SEEK_SET);
}

void FliPlayer::updateTexture()
{
    if (!frameTexture) {
        frameTexture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGB888, SDL_TEXTUREACCESS_STREAMING, 320, 200);
    }

    // Convert 8-bit index frame buffer directly to 32-bit pixel buffer
    for (size_t i = 0; i < 320 * 200; ++i) {
        frameBuffer32[i] = palette[framePixels[i]];
    }

    SDL_UpdateTexture(frameTexture, nullptr, frameBuffer32.data(), 320 * sizeof(uint32_t));
}

void FliPlayer::render() {
    if (frameTexture) {
        SDL_RenderCopy(renderer, frameTexture, nullptr, nullptr);
    }
}
