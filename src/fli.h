#ifndef FLI_H
#define FLI_H

#include <SDL2/SDL.h>
#include <string>
#include <vector>
#include <array>

// Autodesk Animator FLI chunk identifier specifications
enum class FliChunkType : uint16_t {
    COLOR_256 = 4,
    COLOR_64 = 11,
    DELTA_LINES = 12,
    BLACK_FRAME = 13,
    BYTE_RUN = 15,
    RAW_COPY = 16
};

class FliPlayer {
public:
    FliPlayer(SDL_Renderer* renderer);
    ~FliPlayer();

    bool load(const std::string& filename);
    bool update(); // Returns false when the animation sequence has completed
    void render();

private:
    static constexpr uint16_t FLI_MAGIC_ID = 0xAF11;
    static constexpr uint32_t FRAME_DELAY_MS = 70;

    SDL_Renderer* renderer;
    SDL_Texture* frameTexture;

    // Decoded 8-bit index frame buffer
    std::vector<uint8_t> framePixels;
    // 32-bit ARGB/RGB conversion frame buffer
    std::vector<uint32_t> frameBuffer32;
    // 32-bit palette cache
    std::array<uint32_t, 256> palette;

    FILE* fliFile;
    uint16_t totalFrames;
    uint16_t currentFrameIndex;
    uint32_t lastFrameTimeMs;

    void decodeNextFrame();
    void updateTexture();
};

#endif
