#include "mission.h"
#include <fstream>
#include <cstring>
#include <cstdio>

Mission::Mission()
    : startX(1024 << 16),
    startY(1024 << 16),
    extractionX(0),
    extractionY(0),
    targetCount(0)
{
}

bool Mission::load(int missionIndex, Graphics& graphics) {
    // Map stage progression to original binary file lookup arrays
    static const int mission_number[] = { 8, 10, 3, 1, 12, 11, 0, 4, 14, 9, 13, 5, 7, 2, 6 };
    int fileIdx = mission_number[missionIndex - 1];

    char mapName[64];
    std::snprintf(mapName, sizeof(mapName), "assets/FMJK%d.MAP", fileIdx + 1);

    char tilName[64];
    static const char* tile_files[] = {
        "FMJ5.TIL", "FMJ6.TIL", "FMJ7.TIL", "FMJ8.TIL", "FMJ9.TIL", "FMJ10.TIL",
        "FMJE1.TIL", "FMJX1.TIL", "FMJ2-2.TIL", "FMJE2.TIL", "FMJE2.TIL", "FMJE2.TIL",
        "FMJE4.TIL", "FMJE4.TIL", "FMJE4.TIL"
    };
    std::snprintf(tilName, sizeof(tilName), "assets/%s", tile_files[fileIdx]);

    if (!graphics.loadTiles(tilName)) return false;
    if (!loadMapData(mapName, graphics)) return false;

    return true;
}

bool Mission::loadMapData(const std::string& filename, Graphics& graphics) {
    // 1. Populate texture memory inside graphics module first to prevent collision on file streams
    if (!graphics.loadMap(filename)) return false;

    std::ifstream file(filename, std::ios::binary);
    if (!file) return false;

    // 2. Load ENNUM (Enemy Entity Count)
    // Offset calculation: Magic/Header (8 bytes) + Map Tilemap (256 * 256 dwords) + Info offset (24 bytes)
    file.seekg(8 + (256 * 256 * 4) + 24);
    uint32_t ennum;
    file.read(reinterpret_cast<char*>(&ennum), 4);
    if (ennum > 60) {
        ennum = 60; // Cap at max safe configuration size
    }

    // 3. Load ENINFO (Enemy Entity Configurations)
    // Offset calculation: Magic (8 bytes) + Tilemap (256 * 256 dwords) + Header Padding (72 bytes) + Layer layers (30 * 67 dwords)
    uint32_t eninfo_offset = 8 + (256 * 256 * 4) + 72 + (30 * 67 * 4);
    file.seekg(eninfo_offset);

#pragma pack(push, 1)
    struct RawEnInfo {
        int32_t x;
        int32_t y;
        int32_t type;
        int32_t dummy[11];
    };
#pragma pack(pop)

    initialObjects.clear();
    bool playerSpawnFound = false;
    targetCount = 0;

    for (uint32_t i = 0; i < ennum; ++i) {
        RawEnInfo info;
        file.read(reinterpret_cast<char*>(&info), sizeof(info));

        if (info.type == 0) {
            // Player spawn point: ASM logic overrides startup coordinates with the latest evaluated record
            startX = ((info.x << 3) + 4) << 16;
            startY = ((info.y << 3) + 4) << 16;
            playerSpawnFound = true;
        }
        else if (info.type == 99) {
            // Extraction/Return location point
            extractionX = ((info.x << 3) + 4) << 16;
            extractionY = ((info.y << 3) + 4) << 16;
        }
        else if (info.type < 99) {
            // General level object configuration
            Object obj;
            std::memset(&obj, 0, sizeof(obj));
            obj.x = ((info.x << 3) + 4) << 16;
            obj.y = ((info.y << 3) + 4) << 16;
            obj.typeId = info.type;
            obj.frameIdx = info.type;
            obj.state = 1;
            initialObjects.push_back(obj);

            if (info.type == 20) {
                targetCount++;
            }
        }
    }

    // Fallback coordinates if level doesn't configure custom starting points
    if (!playerSpawnFound) {
        startX = (1024 << 16);
        startY = (1024 << 16);
    }

    return true;
}
