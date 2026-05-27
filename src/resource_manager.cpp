#include "resource_manager.h"
#include <fstream>
#include <iostream>

bool ResourceManager::loadAllResources(Graphics& graphics) {
    loadMenuResources(graphics);
    loadHudSprites();
    loadPlayerSprites();
    loadObjectSpriteSheets();

    graphics.loadPCX("assets/CREDIT.GRP", creditSprite);
    loadSpriteSheet("assets/END1.DAT", endingSprites, 15);

    return true;
}

bool ResourceManager::loadPlayerSprites() {
    std::ifstream file("assets/AP_spr_data.bin", std::ios::binary);
    if (!file) {
        std::cerr << "Failed to open AP_spr_data.bin" << std::endl;
        return false;
    }

    // Read file offsets table (45 sprites)
    std::vector<uint32_t> offsets(45);
    file.read(reinterpret_cast<char*>(offsets.data()), 45 * 4);

    uint32_t baseOffset = offsets[0];
    playerSprites.resize(45);

    // Extract individual player animation frame dimensions and raw pixel data
    for (int i = 0; i < 45; ++i) {
        uint32_t relativePos = offsets[i] - baseOffset + 180;
        file.seekg(relativePos);

        file.read(reinterpret_cast<char*>(&playerSprites[i].width), 2);
        file.read(reinterpret_cast<char*>(&playerSprites[i].height), 2);
        playerSprites[i].data.resize(playerSprites[i].width * playerSprites[i].height);
        file.read(reinterpret_cast<char*>(playerSprites[i].data.data()), playerSprites[i].data.size());
    }

    return true;
}

bool ResourceManager::loadSpriteSheet(const std::string& filename, std::vector<Sprite>& outSprites, int count) {
    std::ifstream file(filename, std::ios::binary);
    if (!file) return false;

    // Read the offsets lookup table for the specified sprite sheet
    std::vector<uint32_t> offsets(count);
    file.read(reinterpret_cast<char*>(offsets.data()), count * 4);
    uint32_t baseOffset = offsets[0];

    outSprites.resize(count);
    for (int i = 0; i < count; ++i) {
        if (offsets[i] == 0) continue;
        uint32_t relativePos = offsets[i] - baseOffset + (count * 4);
        file.seekg(relativePos);
        file.read(reinterpret_cast<char*>(&outSprites[i].width), 2);
        file.read(reinterpret_cast<char*>(&outSprites[i].height), 2);
        outSprites[i].data.resize(outSprites[i].width * outSprites[i].height);
        file.read(reinterpret_cast<char*>(outSprites[i].data.data()), outSprites[i].data.size());
    }
    return true;
}

bool ResourceManager::loadObjectSpriteSheets() {
    // Populate entity and effect animation sheets from distinct binary sources
    loadSpriteSheet("assets/flm1_spr_data.bin", objectSprites[GRP_FLM1], 6);
    loadSpriteSheet("assets/flm2_spr_data.bin", objectSprites[GRP_FLM2], 8);
    loadSpriteSheet("assets/flm3_spr_data.bin", objectSprites[GRP_FLM3], 12);
    loadSpriteSheet("assets/flm4_spr_data.bin", objectSprites[GRP_FLM4], 7);
    loadSpriteSheet("assets/chip1_spr_data.bin", objectSprites[GRP_CHIP1], 7);
    loadSpriteSheet("assets/chip2_spr_data.bin", objectSprites[GRP_CHIP2], 6);
    loadSpriteSheet("assets/dust1_spr_data.bin", objectSprites[GRP_DUST1], 6);
    loadSpriteSheet("assets/smk1_spr_data.bin", objectSprites[GRP_SMK1], 10);
    loadSpriteSheet("assets/tbomb_spr_data.bin", objectSprites[GRP_TBOMB], 2);
    loadSpriteSheet("assets/holo_spr_data.bin", objectSprites[GRP_HOLO], 2);
    loadSpriteSheet("assets/mine_spr_data.bin", objectSprites[GRP_MINE], 2);
    loadSpriteSheet("assets/en1_spr_data.bin", objectSprites[GRP_ENEMY1], 25);
    loadSpriteSheet("assets/en2_spr_data.bin", objectSprites[GRP_ENEMY2], 25);
    loadSpriteSheet("assets/en3_spr_data.bin", objectSprites[GRP_ENEMY3], 13);
    loadSpriteSheet("assets/en4_spr_data.bin", objectSprites[GRP_ENEMY4], 17);
    loadSpriteSheet("assets/tank1_spr_data.bin", objectSprites[GRP_TANK1], 3);
    loadSpriteSheet("assets/tank2_spr_data.bin", objectSprites[GRP_TANK2], 3);
    loadSpriteSheet("assets/tank3_spr_data.bin", objectSprites[GRP_TANK3], 3);
    loadSpriteSheet("assets/ttop1_spr_data.bin", objectSprites[GRP_TTOP1], 7);
    loadSpriteSheet("assets/ttop2_spr_data.bin", objectSprites[GRP_TTOP2], 11);
    loadSpriteSheet("assets/ttop3_spr_data.bin", objectSprites[GRP_TTOP3], 7);
    return true;
}

const Sprite& ResourceManager::getGroupSprite(SpriteGroup group, int frame) const {
    auto it = objectSprites.find(group);
    if (it != objectSprites.end() && frame >= 0 && frame < static_cast<int>(it->second.size())) {
        return it->second[frame];
    }
    // Fallback frame when index bounds are exceeded
    return playerSprites[0];
}

bool ResourceManager::loadHudSprites() {
    auto loadBin = [](const std::string& filename, Sprite& sprite) {
        std::ifstream file(filename, std::ios::binary);
        if (!file) {
            std::cerr << "Failed to open HUD bin: " << filename << std::endl;
            return false;
        }
        file.read(reinterpret_cast<char*>(&sprite.width), 2);
        file.read(reinterpret_cast<char*>(&sprite.height), 2);
        sprite.data.resize(sprite.width * sprite.height);
        file.read(reinterpret_cast<char*>(sprite.data.data()), sprite.data.size());
        return true;
    };

    loadBin("assets/panel_spr_data.bin", hudPanel);
    loadBin("assets/mfd1_spr_data.bin", mfd1);
    loadBin("assets/mfd2_spr_data.bin", mfd2);
    loadBin("assets/mfd3_spr_data.bin", mfd3);
    loadBin("assets/compas_spr_data.bin", compass);
    loadSpriteSheet("assets/lamp1_spr_data.bin", lamp1Sprites, 2);
    loadSpriteSheet("assets/lamp2_spr_data.bin", lamp2Sprites, 2);
    loadSpriteSheet("assets/noise_spr_data.bin", noiseSprites, 4);

    return true;
}

bool ResourceManager::loadMenuResources(Graphics& graphics) {
    menuSprites.resize(12);

    // Slice interface buttons out of raw menu PCX documents
    Sprite fmjASprite;
    if (graphics.loadPCX("assets/FMJA.PCX", fmjASprite)) {
        graphics.cutSprite(fmjASprite, 64, 46, 190, 22, menuSprites[0]);
        graphics.cutSprite(fmjASprite, 64, 74, 190, 22, menuSprites[1]);
        graphics.cutSprite(fmjASprite, 64, 102, 190, 22, menuSprites[2]);
        graphics.cutSprite(fmjASprite, 64, 130, 190, 22, menuSprites[3]);
    }
    else {
        std::cerr << "Failed to load menu PCX: FMJA.PCX" << std::endl;
    }

    Sprite fmjA1Sprite;
    if (graphics.loadPCX("assets/FMJA-1.PCX", fmjA1Sprite)) {
        graphics.cutSprite(fmjA1Sprite, 64, 46, 190, 22, menuSprites[4]);
        graphics.cutSprite(fmjA1Sprite, 64, 74, 190, 22, menuSprites[5]);
        graphics.cutSprite(fmjA1Sprite, 64, 102, 190, 22, menuSprites[6]);
        graphics.cutSprite(fmjA1Sprite, 64, 130, 190, 22, menuSprites[7]);
    }
    else {
        std::cerr << "Failed to load menu PCX: FMJA-1.PCX" << std::endl;
    }

    return true;
}
