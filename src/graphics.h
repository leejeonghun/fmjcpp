#ifndef GRAPHICS_H
#define GRAPHICS_H

#include <SDL2/SDL.h>
#include <vector>
#include <string>

// Enum mapping original graphic asset sets to clean group IDs
enum SpriteGroup {
    GRP_NONE = -1,
    GRP_ENEMY1, GRP_ENEMY2, GRP_ENEMY3, GRP_ENEMY4,
    GRP_TANK1, GRP_TANK2, GRP_TANK3, GRP_TTOP1, GRP_TTOP2, GRP_TTOP3,
    GRP_FLM1, GRP_FLM2, GRP_FLM3, GRP_FLM4,
    GRP_DUST1, GRP_SMK1, GRP_CHIP1, GRP_CHIP2,
    GRP_TBOMB, GRP_MINE, GRP_HOLO
};

// Internal sprite resource layout
struct Sprite {
    uint16_t width;
    uint16_t height;
    std::vector<uint8_t> data;
};

// Represents any dynamic game entity (enemies, effects, projectiles)
struct Object {
    uint32_t typeId;
    int32_t x;
    int32_t y;
    int16_t z;
    uint8_t drawAngle;
    uint8_t drawAngleStep;
    uint8_t heading;
    uint8_t headingStep;
    uint8_t turnSpeed;
    uint16_t speed;
    uint16_t defaultSpeed;
    uint16_t altitudeStep;
    uint16_t distToPlayer;
    uint8_t angleToPlayer;
    uint8_t fireCooldown;

    uint32_t frameIdx;
    SpriteGroup spriteGroup;
    const int* script;
    int scriptPos;

    int32_t state;
    int16_t health;
    uint8_t pivotX;
    uint8_t pivotY;

    // Helper utilities for AI and rendering state query
    int getCategory() const { return typeId >> 8; }
};

class Graphics {
public:
    Graphics(SDL_Renderer* renderer);
    ~Graphics();

    void setResolution(int w, int h);
    int getWidth() const { return screenWidth; }
    int getHeight() const { return screenHeight; }

    // Loading handlers for standard resources
    bool loadTiles(const std::string& filename);
    bool loadMap(const std::string& filename);
    bool loadPalette(const std::string& filename);
    bool loadPCXPalette(const std::string& filename);
    bool loadPCX(const std::string& filename, Sprite& outSprite);
    void cutSprite(const Sprite& srcSprite, int x, int y, int w, int h, Sprite& outSprite);

    // Dynamic drawing routines utilizing modern camelCase and clear camX/Y/Alt/Yaw coordinates
    void drawSprite(const Sprite& sprite, int x, int y, bool transparent = true);
    void drawSpritePart(const Sprite& sprite, int x, int y, int srcX, int srcY, int srcW, int srcH, bool transparent = true);
    void drawSpritePerspective(const Sprite& sprite, int worldX, int worldY, int worldZ, int camX, int camY, int camAlt, int camYaw);
    void drawSpriteRotated(const Sprite& sprite, int worldX, int worldY, int worldZ, int camX, int camY, int camAlt, int camYaw, uint8_t drawAngle, int pivotX, int pivotY);
    void drawSpriteRotatedShadow(const Sprite& sprite, int worldX, int worldY, int worldZ, int camX, int camY, int camAlt, int camYaw, uint8_t drawAngle, int pivotX, int pivotY);
    void drawSpriteShadowSC(const Sprite& sprite, int cx, int cy, int camAlt);

    // Mode-7 style floor projections and damage mapping
    void drawFloor(int camX, int camY, int camAlt, int camYaw);
    void drawFloor2(int camX, int camY, int camAlt, int camYaw);
    void damageTile(int cx, int cy);
    void render();

    // Interface and HUD rendering
    void drawRadar(int camX, int camY, int camYaw, const std::vector<Object>& objects);
    void drawCompass(const Sprite& compassSprite, int camYaw);
    void drawText5x5(const std::string& text, int x, int y, uint8_t colorIdx);
    void drawRect(int x, int y, int w, int h, uint8_t colorIdx);

    void clearScreen(uint32_t color);

    bool isCollision(int worldX, int worldY) const;

    void setBlueTint(bool enable) { blueTint = enable; }
    void setRedTint(bool enable) { redTint = enable; }

private:
    // Helper functions for reducing code duplication
    void precalculateDarkerTable(const uint8_t rawPal[768]);
    void writePixel(int idx, uint8_t colorIdx);
    void writeShadowPixel(int idx);
    void drawSpriteRotatedCommon(const Sprite& sprite, int worldX, int worldY, int worldZ, int camX, int camY, int camAlt, int camYaw, uint8_t drawAngle, int pivotX, int pivotY, bool isShadow);

    SDL_Renderer* renderer;
    SDL_Texture* screenTexture = nullptr;

    int screenWidth = 0;
    int screenHeight = 0;

    std::vector<uint32_t> screenBuffer;
    std::vector<uint8_t> screenBufferIdx;

    std::vector<uint8_t> tiles;
    std::vector<uint8_t> dtiles;
    std::vector<uint32_t> texMap;

    SDL_Color palette[256];
    uint8_t darkerTable[256];

    bool blueTint = false;
    bool redTint = false;
};

#endif
