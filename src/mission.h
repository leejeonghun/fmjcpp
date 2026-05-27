#ifndef MISSION_H
#define MISSION_H

#include <string>
#include <vector>
#include "graphics.h"

class Mission {
public:
    Mission();
    
    // Loads the level files based on the sequential mission index
    bool load(int missionIndex, Graphics& graphics);
    
    const std::vector<Object>& getInitialObjects() const { return initialObjects; }
    void getStartPosition(int& outStartX, int& outStartY) const { outStartX = startX; outStartY = startY; }
    void getExtractionPosition(int& outExtractionX, int& outExtractionY) const { outExtractionX = extractionX; outExtractionY = extractionY; }
    int getTargetCount() const { return targetCount; }

private:
    std::vector<Object> initialObjects;
    int startX;
    int startY;
    int extractionX;
    int extractionY;
    int targetCount;
    
    // Parses raw binary level records internally
    bool loadMapData(const std::string& filename, Graphics& graphics);
};

#endif