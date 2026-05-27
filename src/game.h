#ifndef GAME_H
#define GAME_H

#include <SDL2/SDL.h>
#include <string>
#include <vector>
#include "graphics.h"
#include "audio.h"
#include "fli.h"
#include "mission.h"

class Game {
public:
    enum class State {
        INTRO,
        TITLE,
        GAMEPLAY,
        MISSION_CLEAR,
        ENDING
    };

    Game(SDL_Window* window, SDL_Renderer* renderer);
    void run();

private:
    void transitionTo(State newState);
    void handleEvents();
    void update();
    void render();

    // Isolated screen-specific rendering subroutines
    void renderIntro();
    void renderTitle();
    void renderGameplay();
    void renderEnding();

    // Weapon execution and animation updates
    void fireWeapon();
    void updatePlayerAnimation();
    void updateShadowDir();

    void advanceIntro();
    void loadMission(int mIdx);
    void updateMissionGoals();

    // Entity simulation pipelines
    void buildEnemyObjects();
    void advanceObjectScript(Object& obj);
    void updateObjectAI(Object& obj, size_t index);

    // Category-specific discrete AI processors
    void updateProjectileAI(Object& obj);
    void updateInfantryAI(Object& obj, size_t index);
    void updateTurretTopAI(Object& obj, size_t index);
    void updateMissionTargetAI(Object& obj);

    void applyObjectVelocity(Object& obj);
    void syncWorldCoords();

    // Helper subroutines to reduce duplicate code
    void returnToTitle();
    void normalizeHeading(float& angle);
    void spawnLargeExplosionAndDebris(int32_t x, int32_t y);
    void wrapCoordinate(int camCoord, int& objCoord);
    uint8_t calculateAngle(float dx, float dy);
    void calculateDeltaStep(float headingVal, float speed, int32_t& outDx, int32_t& outDy);
    void flushPendingObjects();

    // Factory methods for dynamic entities
    void spawnProjectile(int type, int x, int y, uint8_t dir, uint8_t th, bool is_enemy = false);
    void spawnEffect(int type, int x, int y);
    void spawnDebris(int type, int x, int y, uint8_t th, uint16_t dir);

    void damageGroundArea(int32_t objX, int32_t objY);

    SDL_Window* window;
    SDL_Renderer* renderer;
    bool running;
    int currentWidth = 0;
    int currentHeight = 0;

    Graphics graphics;
    Audio audio;
    FliPlayer fliPlayer;
    Mission mission;

    Uint32 lastLogicTime = 0;

    // Camera space parameters
    int camX = 1024 << 16;
    int camY = 1024 << 16;
    int camAlt = 128;
    int camYaw = 0;
    uint8_t headingInt = 64 * 3;
    float heading = 192.0f;
    float fCamAlt = 128.0f;
    int shadowDX = 0;
    int shadowDY = 0;

    // Player gameplay properties
    int health = 300;
    int weaponSlot = 0;
    int ammo[9] = { 500, 100, 50, 20, 10, 5, 5, 5, 5 };
    int boostFuel = 1000;

    // Decoy hologram metrics
    uint8_t holo_stat = 0;
    int32_t holo_x = 0;
    int32_t holo_y = 0;

    // Control flag and animation index
    uint8_t controls = 0;
    int playerFrame = 0;

    // UI and HUD interface states
    bool showJamNoise = false;
    int targetLampState = -1;
    int extractionLampState = -1;
    bool showTargetWarning = false;
    bool showReturnsMessage = false;

    // Goal and economic metrics
    int missionComplete = 0;
    int goldEarned = 0;
    int totalGold = 0;
    int deathDelay = 0;
    int deathTriggered = 0;

    int killCount = 0;
    int targetCount = 0;
    int targetsDestroyed = 0;
    int killTimer = 2000;
    int retreatTimer = 2000;
    uint8_t goalAchieved[3] = { 0, 0, 0 };

    // Extraction location vectors
    int extractionX = 0;
    int extractionY = 0;
    int extractionDist = 0;
    uint8_t extractionAngle = 0;
    int missionIndex = 1;

    // Input state bitmasks
    static constexpr uint8_t AP_FORW = 0x01;
    static constexpr uint8_t AP_BACK = 0x02;
    static constexpr uint8_t AP_LEFT = 0x04;
    static constexpr uint8_t AP_RIGH = 0x08;
    static constexpr uint8_t AP_FIRE = 0x10;
    static constexpr uint8_t AP_BUST = 0x20;
    static constexpr uint8_t AP_ACCL = 0x40;
    static constexpr uint8_t AP_SHFT = 0x80;

    Sprite titleSprite;

    State state = State::INTRO;
    float ending_y = 220.0f;
    Uint32 lastEndingTick = 0;

    // Sequential animation files list
    std::vector<std::string> fliQueue;
    size_t introFliIdx = 0;
    size_t menuCursor = 0;

    std::vector<Object> objects;
    std::vector<Object> pendingObjects;

    static const int bonus[];
    static const int enemy_gold[];

    // Script instructions curves
    static const int blt1_crs_data[];
    static const int flm1_crs_data[];
    static const int flm2_crs_data[];
    static const int flm3_crs_data[];
    static const int chip1_crs_data[];
    static const int walk1_crs_data[];
    static const int tank_crs_data[];
    static const int flm4_crs_data[];
    static const int dust1_crs_data[];
    static const int smk1_crs_data[];
    static const int chip2_crs_data[];
    static const int blt2_crs_data[];
    static const int missile1_crs_data[];
    static const int walk3_crs_data[];
    static const int walk4_crs_data[];

    static const int weapon_power[];
    static const int weapon_range[];
    static const char* bgm[];
};

#endif
