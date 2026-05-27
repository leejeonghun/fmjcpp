#include "graphics.h"
#include <fstream>
#include <iostream>
#include <cmath>
#include <algorithm>

Graphics::Graphics(SDL_Renderer* renderer)
    : renderer(renderer),
    screenBuffer(320 * 240, 0),
    screenBufferIdx(320 * 240, 0),
    tiles(3000 * 64, 0),
    dtiles(3000 * 64, 0),
    texMap(256 * 256, 0)
{
}

Graphics::~Graphics() {
    if (screenTexture) {
        SDL_DestroyTexture(screenTexture);
    }
}

void Graphics::setResolution(int w, int h) {
    if (screenWidth == w && screenHeight == h) return;
    screenWidth = w;
    screenHeight = h;
    if (screenTexture) {
        SDL_DestroyTexture(screenTexture);
    }
    screenTexture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGB888, SDL_TEXTUREACCESS_STREAMING, w, h);
}

void Graphics::clearScreen(uint32_t color) {
    std::fill(screenBuffer.begin(), screenBuffer.end(), color);
    std::fill(screenBufferIdx.begin(), screenBufferIdx.end(), 0);
}

bool Graphics::loadTiles(const std::string& filename) {
    std::ifstream file(filename, std::ios::binary);
    if (!file) return false;

    // Skip the segment header offset (12-byte head, 768-byte palette, 12000-byte unused/tilemap)
    file.seekg(12 + 768 + 12000);
    file.read(reinterpret_cast<char*>(tiles.data()), tiles.size());

    // Evaluate destroyed tile equivalent paths
    std::string dfilename = filename;
    size_t dotPos = dfilename.find_last_of('.');
    if (dotPos != std::string::npos) {
        std::string base = dfilename.substr(0, dotPos);

        std::string upperBase = base;
        for (char& c : upperBase) c = toupper(c);

        if (upperBase.find("FMJE2") != std::string::npos || upperBase.find("FMJE4") != std::string::npos) {
            base += "-D1";
        }
        else {
            base += "D";
        }
        dfilename = base + dfilename.substr(dotPos);
    }

    std::ifstream dfile(dfilename, std::ios::binary);
    if (dfile) {
        dfile.seekg(12 + 768 + 12000);
        dfile.read(reinterpret_cast<char*>(dtiles.data()), dtiles.size());
    }
    else {
        dtiles = tiles;
    }

    return true;
}

bool Graphics::loadMap(const std::string& filename) {
    std::ifstream file(filename, std::ios::binary);
    if (!file) return false;

    file.seekg(8);
    file.read(reinterpret_cast<char*>(texMap.data()), texMap.size() * sizeof(uint32_t));

    // Clear damage flags on initial map load
    for (int i = 0; i < 256 * 256; ++i) {
        texMap[i] &= ~0x80000000;
    }

    return true;
}

void Graphics::precalculateDarkerTable(const uint8_t rawPal[768]) {
    for (int i = 0; i < 256; ++i) {
        uint8_t currentIdx = static_cast<uint8_t>(i);
        uint32_t sum = rawPal[i * 3 + 0] + rawPal[i * 3 + 1] + rawPal[i * 3 + 2];

        for (int step = 0; step < 7; ++step) {
            int nextIdx = currentIdx + 1;
            if (nextIdx >= 256) break;

            uint32_t nextSum = rawPal[nextIdx * 3 + 0] + rawPal[nextIdx * 3 + 1] + rawPal[nextIdx * 3 + 2];
            if (nextSum > sum) {
                break;
            }
            sum = nextSum;
            currentIdx = static_cast<uint8_t>(nextIdx);
        }
        darkerTable[i] = currentIdx;
    }
    darkerTable[255] = 254;
}

bool Graphics::loadPalette(const std::string& filename) {
    std::ifstream file(filename, std::ios::binary);
    if (!file) return false;

    uint8_t rawPal[768];
    file.read(reinterpret_cast<char*>(rawPal), 768);

    // Translate DOS 6-bit RGB (0-63) to SDL 8-bit standard by multiplying by 4
    for (int i = 0; i < 256; ++i) {
        palette[i].r = static_cast<uint8_t>(rawPal[i * 3 + 0] * 4);
        palette[i].g = static_cast<uint8_t>(rawPal[i * 3 + 1] * 4);
        palette[i].b = static_cast<uint8_t>(rawPal[i * 3 + 2] * 4);
        palette[i].a = 255;
    }

    precalculateDarkerTable(rawPal);
    return true;
}

bool Graphics::loadPCXPalette(const std::string& filename) {
    std::ifstream file(filename, std::ios::binary);
    if (!file) {
        std::cerr << "Failed to open PCX for palette: " << filename << std::endl;
        return false;
    }

    // Read PCX standard palette marker located 769 bytes from the end of file
    file.seekg(-769, std::ios::end);
    uint8_t marker;
    file.read(reinterpret_cast<char*>(&marker), 1);

    if (marker != 0x0C) {
        file.seekg(-768, std::ios::end);
    }

    uint8_t rawPal[768];
    file.read(reinterpret_cast<char*>(rawPal), 768);

    // PCX has native 8-bit RGB (0-255) ranges so no upscale factor is required
    for (int i = 0; i < 256; ++i) {
        palette[i].r = rawPal[i * 3 + 0];
        palette[i].g = rawPal[i * 3 + 1];
        palette[i].b = rawPal[i * 3 + 2];
        palette[i].a = 255;
    }

    precalculateDarkerTable(rawPal);
    return true;
}

bool Graphics::loadPCX(const std::string& filename, Sprite& outSprite) {
    std::ifstream file(filename, std::ios::binary);
    if (!file) {
        std::cerr << "Failed to load PCX raw: " << filename << std::endl;
        return false;
    }

    uint8_t header[128];
    file.read(reinterpret_cast<char*>(header), 128);

    if (header[0] != 10) { // Reject if not ZSoft PCX identifier
        return false;
    }

    int x1 = header[4] | (header[5] << 8);
    int y1 = header[6] | (header[7] << 8);
    int x2 = header[8] | (header[9] << 8);
    int y2 = header[10] | (header[11] << 8);

    outSprite.width = static_cast<uint16_t>(x2 - x1 + 1);
    outSprite.height = static_cast<uint16_t>(y2 - y1 + 1);
    outSprite.data.resize(outSprite.width * outSprite.height);

    int totalBytes = outSprite.width * outSprite.height;
    int count = 0;

    // Standard PCX Run-length encoding decompression loop
    while (count < totalBytes && !file.eof()) {
        uint8_t ch = static_cast<uint8_t>(file.get());
        if ((ch & 0xC0) == 0xC0) {
            int run = ch & 0x3F;
            uint8_t data = static_cast<uint8_t>(file.get());
            while (run-- && count < totalBytes) {
                outSprite.data[count++] = data;
            }
        }
        else {
            outSprite.data[count++] = ch;
        }
    }
    return true;
}

void Graphics::cutSprite(const Sprite& srcSprite, int x, int y, int w, int h, Sprite& outSprite)
{
    outSprite.width = static_cast<uint16_t>(w);
    outSprite.height = static_cast<uint16_t>(h);
    outSprite.data.resize(w * h);

    for (int sy = 0; sy < h; ++sy) {
        for (int sx = 0; sx < w; ++sx) {
            int srcX = x + sx;
            int srcY = y + sy;
            if (srcX >= 0 && srcX < srcSprite.width && srcY >= 0 && srcY < srcSprite.height) {
                outSprite.data[sy * w + sx] = srcSprite.data[srcY * srcSprite.width + srcX];
            }
            else {
                outSprite.data[sy * w + sx] = 0;
            }
        }
    }
}

inline void Graphics::writePixel(int idx, uint8_t colorIdx) {
    SDL_Color color = palette[colorIdx];
    screenBuffer[idx] = (static_cast<uint32_t>(color.a) << 24) |
        (static_cast<uint32_t>(color.r) << 16) |
        (static_cast<uint32_t>(color.g) << 8) |
        static_cast<uint32_t>(color.b);
    screenBufferIdx[idx] = colorIdx;
}

inline void Graphics::writeShadowPixel(int idx) {
    uint8_t currentIdx = screenBufferIdx[idx];
    uint8_t darkerIdx = darkerTable[currentIdx];
    SDL_Color color = palette[darkerIdx];
    screenBuffer[idx] = (static_cast<uint32_t>(color.a) << 24) |
        (static_cast<uint32_t>(color.r) << 16) |
        (static_cast<uint32_t>(color.g) << 8) |
        static_cast<uint32_t>(color.b);
    screenBufferIdx[idx] = darkerIdx;
}

void Graphics::drawSprite(const Sprite& sprite, int x, int y, bool transparent) {
    drawSpritePart(sprite, x, y, 0, 0, sprite.width, sprite.height, transparent);
}

void Graphics::drawSpritePart(const Sprite& sprite, int x, int y, int srcX, int srcY, int srcW, int srcH, bool transparent) {
    for (int sy = 0; sy < srcH; ++sy) {
        for (int sx = 0; sx < srcW; ++sx) {
            int dx = x + sx;
            int dy = y + sy;
            int ssx = srcX + sx;
            int ssy = srcY + sy;
            if (dx >= 0 && dx < screenWidth && dy >= 0 && dy < screenHeight && ssx >= 0 && ssx < sprite.width && ssy >= 0 && ssy < sprite.height) {
                uint8_t colorIdx = sprite.data[ssy * sprite.width + ssx];
                if (!transparent || colorIdx != 0) {
                    writePixel(dy * screenWidth + dx, colorIdx);
                }
            }
        }
    }
}

void Graphics::drawSpritePerspective(const Sprite& sprite, int worldX, int worldY, int worldZ, int camX, int camY, int camAlt, int camYaw) {
    if (sprite.width == 0 || sprite.height == 0 || sprite.data.empty()) return;

    float angle = (camYaw / 256.0f) * 2.0f * static_cast<float>(M_PI);
    float sined = sinf(angle);
    float cosined = cosf(angle);

    float fdx = static_cast<float>(worldX - camX);
    float fdy = static_cast<float>(worldY - camY);

    float rx = fdx * 128.0f / static_cast<float>(camAlt);
    float ry = fdy * 128.0f / static_cast<float>(camAlt);

    float xxxx = rx * cosined - ry * sined;
    float yyyy = ry * cosined + rx * sined;

    float scale = 128.0f / static_cast<float>(camAlt);

    int cx = 192 + static_cast<int>(xxxx / 65536.0f);
    int cy = 200 + static_cast<int>(yyyy / 65536.0f);

    cy -= static_cast<int>(worldZ * scale);

    int sw = static_cast<int>(sprite.width * scale);
    int sh = static_cast<int>(sprite.height * scale);
    if (sw <= 0 || sh <= 0) return;

    int startX = cx - sw / 2;
    int startY = cy - sh / 2;

    for (int py = 0; py < sh; ++py) {
        for (int px = 0; px < sw; ++px) {
            int tx = startX + px;
            int ty = startY + py;
            if (tx >= 64 && tx < 320 && ty >= 0 && ty < 240) {
                int src_x = px * static_cast<int>(sprite.width) / sw;
                int src_y = py * static_cast<int>(sprite.height) / sh;
                uint8_t colorIdx = sprite.data[src_y * sprite.width + src_x];
                if (colorIdx != 0) {
                    writePixel(ty * 320 + tx, colorIdx);
                }
            }
        }
    }
}

void Graphics::drawSpriteRotatedCommon(const Sprite& sprite, int worldX, int worldY, int worldZ, int camX, int camY, int camAlt, int camYaw, uint8_t drawAngle, int pivotX, int pivotY, bool isShadow) {
    if (sprite.width == 0 || sprite.height == 0 || sprite.data.empty()) return;

    float angle_eye = (camYaw / 256.0f) * 2.0f * static_cast<float>(M_PI);
    float s_eye = sinf(angle_eye);
    float c_eye = cosf(angle_eye);

    float fdx = static_cast<float>(worldX - camX);
    float fdy = static_cast<float>(worldY - camY);

    float rx = fdx * 128.0f / static_cast<float>(camAlt);
    float ry = fdy * 128.0f / static_cast<float>(camAlt);

    float cx_f = rx * c_eye - ry * s_eye;
    float cy_f = ry * c_eye + rx * s_eye;

    int cx = 192 + static_cast<int>(cx_f / 65536.0f);
    int cy = 200 + static_cast<int>(cy_f / 65536.0f);

    float angle_rot = (((camYaw + drawAngle) & 255) / 256.0f) * 2.0f * static_cast<float>(M_PI);
    float s_rot = sinf(angle_rot);
    float c_rot = cosf(angle_rot);

    float scale = static_cast<float>(camAlt) / 128.0f;
    int hwide = 23;

    for (int py = -hwide; py <= hwide; ++py) {
        int screen_y = cy + py;
        if (screen_y < 0 || screen_y >= 240) continue;

        for (int px = -hwide; px <= hwide; ++px) {
            int screen_x = cx + px;
            if (screen_x < 64 || screen_x >= 320) continue;

            float scaled_px = px * scale;
            float scaled_py = py * scale;

            int tex_u = static_cast<int>(scaled_px * c_rot + scaled_py * s_rot) + pivotX;
            int tex_v = static_cast<int>(scaled_py * c_rot - scaled_px * s_rot) + pivotY;

            if (tex_u >= 0 && tex_u < sprite.width && tex_v >= 0 && tex_v < sprite.height) {
                uint8_t colorIdx = sprite.data[tex_v * sprite.width + tex_u];
                if (colorIdx != 0) {
                    int idx = screen_y * 320 + screen_x;
                    if (isShadow) {
                        writeShadowPixel(idx);
                    }
                    else {
                        writePixel(idx, colorIdx);
                    }
                }
            }
        }
    }
}

void Graphics::drawSpriteRotated(const Sprite& sprite, int worldX, int worldY, int worldZ, int camX, int camY, int camAlt, int camYaw, uint8_t drawAngle, int pivotX, int pivotY) {
    drawSpriteRotatedCommon(sprite, worldX, worldY, worldZ, camX, camY, camAlt, camYaw, drawAngle, pivotX, pivotY, false);
}

void Graphics::drawSpriteRotatedShadow(const Sprite& sprite, int worldX, int worldY, int worldZ, int camX, int camY, int camAlt, int camYaw, uint8_t drawAngle, int pivotX, int pivotY) {
    drawSpriteRotatedCommon(sprite, worldX, worldY, worldZ, camX, camY, camAlt, camYaw, drawAngle, pivotX, pivotY, true);
}

void Graphics::drawSpriteShadowSC(const Sprite& sprite, int cx, int cy, int camAlt) {
    float scale = 128.0f / camAlt;
    int sw = static_cast<int>(sprite.width * scale);
    int sh = static_cast<int>(sprite.height * scale);

    if (sw <= 0 || sh <= 0) return;

    for (int py = 0; py < sh; ++py) {
        for (int px = 0; px < sw; ++px) {
            int target_x = cx - sw / 2 + px;
            int target_y = cy - sh / 2 + py;

            if (target_x >= 0 && target_x < 320 && target_y >= 0 && target_y < 240) {
                int src_x = px * sprite.width / sw;
                int src_y = py * sprite.height / sh;

                uint8_t colorIdx = sprite.data[src_y * sprite.width + src_x];
                if (colorIdx != 0) {
                    writeShadowPixel(target_y * 320 + target_x);
                }
            }
        }
    }
}

void Graphics::drawFloor(int camX, int camY, int camAlt, int camYaw) {
    float angle = (camYaw / 256.0f) * 2.0f * static_cast<float>(M_PI);
    float sined = sinf(angle);
    float cosined = cosf(angle);

    float feye_x = camX / 65536.0f;
    float feye_y = camY / 65536.0f;
    float feye_z = camAlt / 128.0f;

    for (int x = 64; x < 320; ++x) {
        float bp = static_cast<float>(x - 192);

        float xx = bp * feye_z;
        float ey1 = -200.0f * feye_z;
        float ey2 = 39.0f * feye_z;

        float x1 = xx * cosined + ey1 * sined + feye_x;
        float y1 = ey1 * cosined - xx * sined + feye_y;
        float x2 = xx * cosined + ey2 * sined + feye_x;
        float y2 = ey2 * cosined - xx * sined + feye_y;

        float dx = (x2 - x1) / 240.0f;
        float dy = (y2 - y1) / 240.0f;

        float cur_x = x1;
        float cur_y = y1;

        for (int y = 0; y < 240; ++y) {
            int mx = (static_cast<int>(cur_x) >> 3) & 255;
            int my = (static_cast<int>(cur_y) >> 3) & 255;
            int tx = static_cast<int>(cur_x) & 7;
            int ty = static_cast<int>(cur_y) & 7;

            uint32_t tileData = texMap[my * 256 + mx];
            uint32_t tileIdx = tileData & 0xFFF;

            uint8_t colorIdx = (tileData & 0x80000000)
                ? dtiles[tileIdx * 64 + ty * 8 + tx]
                : tiles[tileIdx * 64 + ty * 8 + tx];

            writePixel(y * 320 + x, colorIdx);

            cur_x += dx;
            cur_y += dy;
        }
    }
}

void Graphics::drawFloor2(int camX, int camY, int camAlt, int camYaw) {
    if (camAlt > 133) return;

    float angle = (camYaw / 256.0f) * 2.0f * static_cast<float>(M_PI);
    float sined = sinf(angle);
    float cosined = cosf(angle);

    float feye_x = camX / 65536.0f;
    float feye_y = camY / 65536.0f;
    float feye_z = camAlt / 128.0f;

    for (int x = 176; x < 208; ++x) {
        float bp = static_cast<float>(x - 192);

        float xx = bp * feye_z;
        float ey1 = -17.0f * feye_z;
        float ey2 = 15.0f * feye_z;

        float x1 = xx * cosined + ey1 * sined + feye_x;
        float y1 = ey1 * cosined - xx * sined + feye_y;
        float x2 = xx * cosined + ey2 * sined + feye_x;
        float y2 = ey2 * cosined - xx * sined + feye_y;

        float dx = (x2 - x1) / 32.0f;
        float dy = (y2 - y1) / 32.0f;

        float cur_x = x1;
        float cur_y = y1;

        for (int y = 184; y < 216; ++y) {
            int mx = (static_cast<int>(cur_x) >> 3) & 255;
            int my = (static_cast<int>(cur_y) >> 3) & 255;
            int tx = static_cast<int>(cur_x) & 7;
            int ty = static_cast<int>(cur_y) & 7;

            uint32_t tileData = texMap[my * 256 + mx];

            if (tileData & 0x00200000) {
                uint32_t tileIdx;
                uint8_t colorIdx;

                if (tileData & 0x00400000) {
                    tileIdx = (tileData >> 12) & 0xFF;
                    colorIdx = tiles[tileIdx * 64 + ty * 8 + tx];
                }
                else {
                    tileIdx = tileData & 0xFFF;
                    colorIdx = (tileData & 0x80000000)
                        ? dtiles[tileIdx * 64 + ty * 8 + tx]
                        : tiles[tileIdx * 64 + ty * 8 + tx];
                }

                if (colorIdx != 0) {
                    writePixel(y * 320 + x, colorIdx);
                }
            }
            cur_x += dx;
            cur_y += dy;
        }
    }
}

void Graphics::damageTile(int cx, int cy) {
    if (cx >= 0 && cx < 256 && cy >= 0 && cy < 256) {
        texMap[cy * 256 + cx] |= 0x80000000;
    }
}

static const uint8_t GREEN_TABLE[256] = {
    144,145,146,147,148,149,150,151,152,153,154,155,156,157,158,159,
    144,145,146,147,148,149,150,151,152,153,154,155,156,157,158,159,
    144,145,146,147,148,149,150,151,152,153,154,155,156,157,158,159,
    144,145,146,147,148,149,150,151,152,153,154,155,156,157,158,159,
    144,145,146,147,148,149,150,151,152,153,154,155,156,157,158,159,
    144,145,146,147,148,149,150,151,152,153,154,155,156,157,158,159,
    144,145,146,147,148,149,150,151,152,153,154,155,156,157,158,159,
    144,145,146,147,148,149,150,151,152,153,154,155,156,157,158,159,
    144,145,146,147,148,149,150,151,152,153,154,155,156,157,158,159,
    144,145,146,147,148,149,150,151,152,153,154,155,156,157,158,159,

    144,144,145,145,146,146,147,147,148,148,149,149,150,150,151,151,
    152,152,153,153,154,154,155,155,156,156,157,157,158,158,159,159,

    144,144,145,145,146,146,147,147,148,148,149,149,150,150,151,151,
    152,152,153,153,154,154,155,155,156,156,157,157,158,158,159,159,

    144,144,145,145,146,146,147,147,148,148,149,149,150,150,151,151,
    152,152,153,153,154,154,155,155,156,156,157,157,158,158,159,159
};

void Graphics::drawRadar(int camX, int camY, int camYaw, const std::vector<Object>& objects) {
    float feye_x = camX / 65536.0f;
    float feye_y = camY / 65536.0f;
    float angle = (camYaw / 256.0f) * 2.0f * static_cast<float>(M_PI);
    float sined = sinf(angle);
    float cosined = cosf(angle);

    const float feye_z = 1500.0f / 128.0f;

    for (int x = 0; x < 58; ++x) {
        float bp = static_cast<float>(x - 29);

        float xx = bp * feye_z;
        float ey1 = -29.0f * feye_z;
        float ey2 = 29.0f * feye_z;

        float x1 = xx * cosined + ey1 * sined + feye_x;
        float y1 = ey1 * cosined - xx * sined + feye_y;
        float x2 = xx * cosined + ey2 * sined + feye_x;
        float y2 = ey2 * cosined - xx * sined + feye_y;

        float dx = (x2 - x1) / 57.0f;
        float dy = (y2 - y1) / 57.0f;
        float cur_x = x1;
        float cur_y = y1;

        for (int y = 0; y < 57; ++y) {
            int mx = (static_cast<int>(cur_x) >> 3) & 255;
            int my = (static_cast<int>(cur_y) >> 3) & 255;
            int tx = static_cast<int>(cur_x) & 7;
            int ty = static_cast<int>(cur_y) & 7;

            uint32_t tileData = texMap[my * 256 + mx];
            uint32_t tileIdx = tileData & 0xFFF;

            uint8_t colorIdx = (tileData & 0x80000000)
                ? dtiles[tileIdx * 64 + ty * 8 + tx]
                : tiles[tileIdx * 64 + ty * 8 + tx];

            uint8_t mappedIdx = GREEN_TABLE[colorIdx];

            int target_x = 3 + x;
            int target_y = 3 + y;
            if (target_x < 320 && target_y < 240) {
                writePixel(target_y * 320 + target_x, mappedIdx);
            }

            cur_x += dx;
            cur_y += dy;
        }
    }

    for (const auto& obj : objects) {
        if (obj.state == 0) continue;
        if ((obj.typeId & 0x100) != 0) continue;

        float obj_x = obj.x / 65536.0f;
        float obj_y = obj.y / 65536.0f;

        float dx = obj_x - feye_x;
        float dy = obj_y - feye_y;

        float screen_dx = (dx * cosined - dy * sined) / feye_z;
        float screen_dy = (dx * sined + dy * cosined) / feye_z;

        int radar_x = static_cast<int>(floorf(screen_dx)) + 32;
        int radar_y = static_cast<int>(floorf(screen_dy)) + 32;

        if (radar_x >= 3 && radar_x <= 60 && radar_y >= 3 && radar_y <= 59) {

            if (obj.typeId == 0x800) {
                if (((SDL_GetTicks() / 71) % 2) != 0) continue;
            }

            uint8_t colorIdx = (obj.health <= 0) ? 11 : 9;
            writePixel(radar_y * 320 + radar_x, colorIdx);
        }
    }
}

static const uint8_t font5[][5] = {
    {0x00, 0x00, 0x00, 0x00, 0x00}, //  
    {0x20, 0x00, 0x20, 0x20, 0x20}, // !
    {0x00, 0x00, 0x00, 0x50, 0x50}, // "
    {0x50, 0xF8, 0x50, 0xF8, 0x50}, // #
    {0x20, 0x70, 0x20, 0x70, 0x20}, // $
    {0x88, 0x40, 0x20, 0x10, 0x88}, // %
    {0x30, 0x58, 0x20, 0x50, 0x20}, // &
    {0x00, 0x00, 0x00, 0x20, 0x20}, // '
    {0x20, 0x40, 0x40, 0x40, 0x20}, // (
    {0x20, 0x10, 0x10, 0x10, 0x20}, // )
    {0x88, 0x50, 0xA8, 0x50, 0x88}, // *
    {0x20, 0x20, 0xF8, 0x20, 0x20}, // +
    {0x80, 0x40, 0x00, 0x00, 0x00}, // ,
    {0x00, 0x00, 0x70, 0x00, 0x00}, // -
    {0x40, 0x00, 0x00, 0x00, 0x00}, // .
    {0x80, 0x40, 0x20, 0x10, 0x08}, // /
    {0x70, 0x88, 0x88, 0x88, 0x70}, // 0
    {0x70, 0x20, 0x20, 0x60, 0x20}, // 1
    {0xF0, 0x40, 0x20, 0x90, 0x60}, // 2
    {0xE0, 0x10, 0x60, 0x10, 0xE0}, // 3
    {0x10, 0x10, 0xF0, 0x90, 0x90}, // 4
    {0x70, 0x08, 0xF0, 0x80, 0xF8}, // 5
    {0x70, 0x88, 0xF0, 0x80, 0x70}, // 6
    {0x40, 0x40, 0x20, 0x10, 0xF8}, // 7
    {0x70, 0x88, 0x70, 0x88, 0x70}, // 8
    {0x10, 0x08, 0x78, 0x88, 0x70}, // 9
    {0x40, 0x00, 0x00, 0x40, 0x00}, // :
    {0x40, 0x20, 0x00, 0x20, 0x00}, // ;
    {0x10, 0x20, 0x40, 0x20, 0x10}, // <
    {0x00, 0x70, 0x00, 0x70, 0x00}, // =
    {0x40, 0x20, 0x10, 0x20, 0x40}, // >
    {0x10, 0x00, 0x10, 0x48, 0x30}, // ?
    {0x70, 0x80, 0xB0, 0xB0, 0x60}, // @
    {0x88, 0xF8, 0x88, 0x50, 0x20}, // A
    {0xF0, 0x88, 0xF0, 0x88, 0xF0}, // B
    {0x78, 0x80, 0x80, 0x80, 0x78}, // C
    {0xF0, 0x88, 0x88, 0x88, 0xF0}, // D
    {0xF8, 0x80, 0xF0, 0x80, 0xF8}, // E
    {0x80, 0x80, 0xF0, 0x80, 0xF8}, // F
    {0x70, 0x88, 0x98, 0x80, 0x78}, // G
    {0x88, 0x88, 0xF8, 0x88, 0x88}, // H
    {0x70, 0x20, 0x20, 0x20, 0x70}, // I
    {0x60, 0x90, 0x10, 0x10, 0x38}, // J
    {0x88, 0x90, 0xE0, 0x90, 0x88}, // K
    {0xF8, 0x80, 0x80, 0x80, 0x80}, // L
    {0x88, 0xA8, 0xA8, 0xD8, 0x88}, // M
    {0x88, 0x98, 0xA8, 0xC8, 0x88}, // N
    {0x70, 0x88, 0x88, 0x88, 0x70}, // O
    {0x80, 0x80, 0xF0, 0x88, 0xF0}, // P
    {0x78, 0xA8, 0x88, 0x88, 0x70}, // Q
    {0x90, 0xA0, 0xF0, 0x88, 0xF0}, // R
    {0xF0, 0x08, 0x70, 0x80, 0x78}, // S
    {0x20, 0x20, 0x20, 0x20, 0xF8}, // T
    {0x70, 0x88, 0x88, 0x88, 0x88}, // U
    {0x20, 0x50, 0x88, 0x88, 0x88}, // V
    {0x88, 0xD8, 0xA8, 0xA8, 0x88}, // W
    {0x88, 0x50, 0x20, 0x50, 0x88}, // X
    {0x20, 0x20, 0x20, 0x50, 0x88}, // Y
    {0xF8, 0x40, 0x20, 0x10, 0xF8}, // Z
    {0x70, 0x40, 0x40, 0x40, 0x70}, // [
    {0x08, 0x10, 0x20, 0x40, 0x80}, // Backslash
    {0x70, 0x10, 0x10, 0x10, 0x70}, // ]
    {0x00, 0x00, 0x88, 0x50, 0x20}, // ^
    {0xF8, 0x00, 0x00, 0x00, 0x00}, // _
    {0x00, 0x00, 0x00, 0x20, 0x20}, // `
    {0x88, 0xF8, 0x88, 0x50, 0x20}, // a
    {0xF0, 0x88, 0xF0, 0x88, 0xF0}, // b
    {0x78, 0x80, 0x80, 0x80, 0x78}, // c
    {0xF0, 0x88, 0x88, 0x88, 0xF0}, // d
    {0xF8, 0x80, 0xF0, 0x80, 0xF8}, // e
    {0x80, 0x80, 0xF0, 0x80, 0xF0}, // f
    {0x70, 0x88, 0x98, 0x80, 0x78}, // g
    {0x88, 0x88, 0xF8, 0x88, 0x88}, // h
    {0x70, 0x20, 0x20, 0x20, 0x70}, // i
    {0x60, 0x90, 0x10, 0x10, 0x38}, // j
    {0x88, 0x90, 0xE0, 0x90, 0x88}, // k
    {0xF8, 0x80, 0x80, 0x80, 0x80}, // l
    {0x88, 0xA8, 0xA8, 0xD8, 0x88}, // m
    {0x88, 0x98, 0xA8, 0xC8, 0x88}, // n
    {0x70, 0x88, 0x88, 0x88, 0x70}, // o
    {0x80, 0x80, 0xF0, 0x88, 0xF0}, // p
    {0x78, 0xA8, 0x88, 0x88, 0x70}, // q
    {0x90, 0xA0, 0xF0, 0x88, 0xF0}, // r
    {0xF0, 0x08, 0x70, 0x80, 0x78}, // s
    {0x20, 0x20, 0x20, 0x20, 0xF8}, // t
    {0x70, 0x88, 0x88, 0x88, 0x88}, // u
    {0x20, 0x50, 0x88, 0x88, 0x88}, // v
    {0x88, 0xD8, 0xA8, 0xA8, 0x88}, // w
    {0x88, 0x50, 0x20, 0x50, 0x88}, // x
    {0x20, 0x20, 0x20, 0x50, 0x88}, // y
    {0xF8, 0x40, 0x20, 0x10, 0xF8}, // z
    {0x30, 0x20, 0x60, 0x20, 0x30}, // {
    {0x20, 0x20, 0x00, 0x20, 0x20}, // |
    {0x60, 0x20, 0x30, 0x20, 0x60}, // }
    {0x00, 0x00, 0x90, 0x68, 0x00}  // ~
};

void Graphics::drawText5x5(const std::string& text, int x, int y, uint8_t colorIdx) {
    SDL_Color color = palette[colorIdx];
    uint32_t color32 = (color.a << 24) | (color.r << 16) | (color.g << 8) | color.b;

    int curX = x;
    for (char c : text) {
        if (c < 32 || c > 126) continue;

        const uint8_t* glyph = font5[c - 32];

        for (int row = 0; row < 5; ++row) {
            uint8_t rowBits = glyph[4 - row];

            for (int col = 0; col < 5; ++col) {
                if (rowBits & (0x80 >> col)) {
                    int dx = curX + col;
                    int dy = y + row;

                    if (dx >= 0 && dx < 320 && dy >= 0 && dy < 240) {
                        writePixel(dy * 320 + dx, colorIdx);
                    }
                }
            }
        }
        curX += 6;
    }
}

void Graphics::drawRect(int x, int y, int w, int h, uint8_t colorIdx) {
    for (int dy = 0; dy < h; ++dy) {
        for (int dx = 0; dx < w; ++dx) {
            int tx = x + dx;
            int ty = y + dy;
            if (tx >= 0 && tx < 320 && ty >= 0 && ty < 240) {
                writePixel(ty * 320 + tx, colorIdx);
            }
        }
    }
}

void Graphics::drawCompass(const Sprite& compassSprite, int camYaw) {
    int bx = (-((camYaw + 2) & 0xFF) + 32) & 0xFF;
    int srcX = (137 * bx) / 255;

    drawSpritePart(compassSprite, 16, 65, srcX, 0, 41, 7, true);
}

bool Graphics::isCollision(int worldX, int worldY) const {
    int mx = (worldX >> 16) >> 3;
    int my = (worldY >> 16) >> 3;
    if (mx < 0 || mx >= 256 || my < 0 || my >= 256) return true;

    return (texMap[my * 256 + mx] & 0x00100000) != 0;
}

void Graphics::render() {
    SDL_UpdateTexture(screenTexture, NULL, screenBuffer.data(), screenWidth * static_cast<int>(sizeof(uint32_t)));

    if (blueTint) {
        SDL_SetTextureColorMod(screenTexture, 0, 0, 255);
    }
    else if (redTint) {
        SDL_SetTextureColorMod(screenTexture, 255, 0, 0);
    }
    else {
        SDL_SetTextureColorMod(screenTexture, 255, 255, 255);
    }

    SDL_RenderCopy(renderer, screenTexture, NULL, NULL);
}
