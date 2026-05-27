#ifndef RESOURCE_MANAGER_H
#define RESOURCE_MANAGER_H

#include <string>
#include <vector>
#include <map>
#include "graphics.h"

class ResourceManager {
public:
    // Singleton access point
    static ResourceManager& getInstance() {
        static ResourceManager instance;
        return instance;
    }

    // Loads all game resources from the disk
    bool loadAllResources(Graphics& graphics);

    // Asset accessor methods
    const Sprite& getMenuSprite(int index) const { return menuSprites.at(index); }

    const Sprite& getHudPanel() const { return hudPanel; }
    const Sprite& getMfd1() const { return mfd1; }
    const Sprite& getMfd2() const { return mfd2; }
    const Sprite& getMfd3() const { return mfd3; }
    const Sprite& getCompass() const { return compass; }
    const Sprite& getPlayerSprite(int frame) const { return playerSprites.at(frame); }
    const Sprite& getLamp1Sprite(int frame) const { return lamp1Sprites.at(frame); }
    const Sprite& getLamp2Sprite(int frame) const { return lamp2Sprites.at(frame); }
    const Sprite& getNoiseSprite(int frame) const { return noiseSprites.at(frame); }
    const Sprite& getGroupSprite(SpriteGroup group, int frame) const;

    const std::vector<Sprite>& getEndingSprites() const { return endingSprites; }
    const Sprite& getCreditSprite() const { return creditSprite; }

private:
    ResourceManager() {}

    // Internal parsing and loading helper functions
    bool loadMenuResources(Graphics& graphics);
    bool loadHudSprites();
    bool loadPlayerSprites();
    bool loadObjectSpriteSheets();
    bool loadSpriteSheet(const std::string& filename, std::vector<Sprite>& outSprites, int count);

    // Sprite memory arrays and mappings
    std::map<SpriteGroup, std::vector<Sprite>> objectSprites;
    std::vector<Sprite> menuSprites;

    // HUD and interface panel assets
    Sprite hudPanel;
    Sprite mfd1;
    Sprite mfd2;
    Sprite mfd3;
    Sprite compass;
    std::vector<Sprite> lamp1Sprites;
    std::vector<Sprite> lamp2Sprites;
    std::vector<Sprite> noiseSprites;
    std::vector<Sprite> playerSprites;

    // Ending cutscene and credits assets
    std::vector<Sprite> endingSprites;
    Sprite creditSprite;
};

#endif
