#include "game.h"
#include "resource_manager.h"
#include <iostream>
#include <algorithm>
#include <cmath>
#include <cstring>

// Original script-based instruction codes inside raw data curves
static const int CMD_SFX = 200;
static const int CMD_KILL = 209;
static const int CMD_SPAWN_FLM = 210;
static const int CMD_GOTO = 230;
static const int CMD_JMP_IF_STAT = 231;
static const int CMD_JMP_IF_DEAD = 232;

const int Game::blt1_crs_data[] = { 0,1,0,1,0,1,0,1,0,1,0,1,0,1, 0,1,0,1,0,1,0,1,0,1,0,1,0,1, CMD_SFX,2, CMD_SPAWN_FLM,1, CMD_KILL, -1 };
const int Game::flm1_crs_data[] = { 0,1,2,3,4,5, CMD_KILL, -1 };
const int Game::flm2_crs_data[] = { 0,1,2,3,4,5,6,7, CMD_KILL, -1 };
const int Game::flm3_crs_data[] = { CMD_SFX,10, 0,1,2,3,4,5,6,7,8,9,10,11, CMD_KILL, -1 };
const int Game::chip1_crs_data[] = { 0,1,2,3,4,5,6,0,1,2,3,4,5,6, CMD_SPAWN_FLM,4, CMD_KILL, -1 };
const int Game::flm4_crs_data[] = { 0, 1, 2, 3, 4, 5, 6, CMD_KILL, -1 };
const int Game::dust1_crs_data[] = { 0, 1, 2, 3, 4, 5, CMD_KILL, -1 };
const int Game::smk1_crs_data[] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, CMD_KILL, -1 };
const int Game::chip2_crs_data[] = { 0, 1, 2, 3, 4, 5, 0, 1, 2, 3, 4, 5, CMD_SPAWN_FLM, 4, CMD_KILL, -1 };
const int Game::blt2_crs_data[] = { 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, CMD_GOTO, 0, -1 };
const int Game::missile1_crs_data[] = {
    CMD_SPAWN_FLM, 3, 0, CMD_SPAWN_FLM, 3, 1, CMD_SPAWN_FLM, 3, 0, CMD_SPAWN_FLM, 3, 1,
    CMD_SPAWN_FLM, 3, 0, CMD_SPAWN_FLM, 3, 1, CMD_SPAWN_FLM, 3, 0, CMD_SPAWN_FLM, 3, 1,
    CMD_SPAWN_FLM, 3, 0, CMD_SPAWN_FLM, 3, 1, CMD_SPAWN_FLM, 3, 0, CMD_SPAWN_FLM, 3, 1,
    CMD_SPAWN_FLM, 3, 0, CMD_SPAWN_FLM, 3, 1, CMD_SPAWN_FLM, 3, 0, CMD_SPAWN_FLM, 3, 1,
    CMD_SPAWN_FLM, 3, 0, CMD_SPAWN_FLM, 3, 1, CMD_SPAWN_FLM, 3, 0, CMD_SPAWN_FLM, 3, 1,
    CMD_SPAWN_FLM, 3, 0, CMD_SPAWN_FLM, 3, 1, CMD_SPAWN_FLM, 3, 0, CMD_SPAWN_FLM, 3, 1,
    CMD_SPAWN_FLM, 3, 0, CMD_SPAWN_FLM, 3, 1, CMD_SPAWN_FLM, 3, 0, CMD_SPAWN_FLM, 3, 1,
    CMD_SPAWN_FLM, 2, CMD_SFX, 10, CMD_KILL, -1
};

const int Game::walk1_crs_data[] = {
    CMD_JMP_IF_DEAD, 22,
    0, 1, 2, 3, 4, 5,
    CMD_JMP_IF_STAT, 2, 14,
    CMD_GOTO, 0,
    -1,
    18, 19, 20, 21, 22, 23,
    CMD_GOTO, 0,
    CMD_SPAWN_FLM, 2,
    24, -1
};

const int Game::walk3_crs_data[] = {
    CMD_JMP_IF_DEAD, 22,
    0, 1, 2, 3, 4, 5,
    CMD_JMP_IF_STAT, 2, 14,
    CMD_GOTO, 0,
    -1,
    6, 7, 8, 9, 10, 11,
    CMD_GOTO, 0,
    CMD_SPAWN_FLM, 2,
    12, -1
};

const int Game::walk4_crs_data[] = {
    CMD_JMP_IF_DEAD, 22,
    0, 1, 2, 3, 4, 5,
    CMD_JMP_IF_STAT, 2, 14,
    CMD_GOTO, 0,
    -1,
    8, 9, 10, 11, 12, 13,
    CMD_GOTO, 0,
    CMD_SPAWN_FLM, 2,
    16, -1
};

const int Game::tank_crs_data[] = {
    0, 1,
    CMD_JMP_IF_DEAD, 6,
    CMD_GOTO, 0,
    CMD_SPAWN_FLM, 2,
    2, -1
};

const int Game::weapon_power[] = { 3, 2, 4, 25, 100, 80, 150, 80, 60 };
const int Game::weapon_range[] = { 0, 0, 20, 20, 80, 80, 40, 15, 15 };
const int Game::bonus[] = { 600, 700, 800, 850, 900, 1200, 1250, 1350, 1400, 1500, 1600, 1700, 2000, 2000, 0 };
const int Game::enemy_gold[] = { 35, 35, 40, 40, 30, 35, 30, 25, 20, 25 };

const char* Game::bgm[] = {
    "FM002.MOD", "FM000.MOD", "FM003.MOD", "FM004.MOD", "FM008.MOD",
    "FM005.MOD", "FM007.MOD", "FM010.MOD", "FM008.MOD", "FM013.MOD",
    "FM008.MOD", "FM002.MOD", "FM014.MOD", "FM018.MOD", "FM101.MOD"
};

Game::Game(SDL_Window* window, SDL_Renderer* renderer)
    : window(window), renderer(renderer), running(true), graphics(renderer), fliPlayer(renderer) {

    transitionTo(State::INTRO);

    ResourceManager::getInstance().loadAllResources(graphics);
    graphics.loadPCX("assets/FMJA.PCX", titleSprite);

    const char* wavfn[] = {
        "FMJ01.WAV", "FMJ02.WAV", "FMJ03.WAV", "CLICK3.WAV", "CLICK10.WAV",
        "SFX01.WAV", "SIREN1.WAV", "BAND.WAV", "SLD.WAV", "GUNDRY.WAV",
        "EXPLO1.WAV", "EXPLO2.WAV", "SGUNSH.WAV", "SGUNAC.WAV", "CANNON.WAV",
        "FTHROW.WAV", "MCHGUN.WAV", "DROP.WAV", "BUSTON.WAV", "BUST.WAV"
    };
    for (int i = 0; i < 20; ++i) {
        char path[64];
        std::snprintf(path, sizeof(path), "assets/%s", wavfn[i]);
        audio.loadSound(static_cast<SoundId>(i), path);
    }

    audio.loadMusic("assets/FM001.MOD");
    audio.playMusic();

    fliQueue = { "MIRE.FLI", "FMJOPEN1.FLI", "FMJOPEN2.FLI", "FMJOPEN3.FLI", "FMJOPEN4.FLI" };
    introFliIdx = 0;
    fliPlayer.load("assets/" + fliQueue[introFliIdx]);
}

void Game::loadMission(int mIdx) {
    missionIndex = mIdx;
    objects.clear();
    pendingObjects.clear();

    if (mission.load(missionIndex, graphics)) {
        mission.getStartPosition(camX, camY);
        mission.getExtractionPosition(extractionX, extractionY);
        targetCount = mission.getTargetCount();

        objects = mission.getInitialObjects();
        buildEnemyObjects();

        camAlt = 128;
        headingInt = 192;
        camYaw = 0;

        heading = 192.0f;
        fCamAlt = 128.0f;

        health = 300;
        boostFuel = 1000;

        holo_stat = 0;
        holo_x = 0;
        holo_y = 0;

        ammo[0] = 500; ammo[1] = 100; ammo[2] = 50;
        ammo[3] = 20; ammo[4] = 10; ammo[5] = 5;
        ammo[6] = 5; ammo[7] = 5; ammo[8] = 5;

        killCount = 0;
        goldEarned = 0;
        targetsDestroyed = 0;
        killTimer = 2000;
        retreatTimer = 2000;
        goalAchieved[0] = 0;
        goalAchieved[1] = 0;
        goalAchieved[2] = 0;
        missionComplete = 0;
        deathDelay = 0;
        deathTriggered = 0;
        controls = 0;

        showJamNoise = false;
        targetLampState = -1;
        extractionLampState = -1;
        showTargetWarning = false;
        showReturnsMessage = false;

        graphics.setBlueTint(false);
        graphics.setRedTint(false);

        std::string musicPath = "assets/";
        musicPath += bgm[missionIndex - 1];
        audio.loadMusic(musicPath.c_str());
        audio.playMusic();

        lastLogicTime = SDL_GetTicks();
    }
}

void Game::run() {
    Uint32 frameStart;
    int frameTime;
    while (running) {
        frameStart = SDL_GetTicks();
        handleEvents();
        update();
        render();
        frameTime = static_cast<int>(SDL_GetTicks() - frameStart);
        if (frameTime < 16) SDL_Delay(16 - frameTime);
    }
}

void Game::advanceIntro() {
    introFliIdx++;
    if (introFliIdx < fliQueue.size()) {
        fliPlayer.load("assets/" + fliQueue[introFliIdx]);
    }
    else {
        Uint32 startTicks = SDL_GetTicks();
        bool skip = false;

        while (SDL_GetTicks() - startTicks < 2000 && !skip && running) {
            SDL_Event event;
            while (SDL_PollEvent(&event)) {
                if (event.type == SDL_QUIT) {
                    running = false;
                    skip = true;
                }
                else if (event.type == SDL_KEYDOWN) {
                    skip = true;
                }
            }
            SDL_Delay(16);
        }

        returnToTitle();
    }
}

void Game::transitionTo(State newState) {
    state = newState;

    int width, height;
    switch (newState) {
    case State::INTRO:
        width = 320;
        height = 200;
        break;
    case State::TITLE:
        width = 320;
        height = 200;
        graphics.loadPalette("assets/FMJP.P");
        break;
    case State::GAMEPLAY:
    case State::MISSION_CLEAR:
        width = 320;
        height = 240;
        graphics.loadPalette("assets/FMJ.PAL");
        break;
    case State::ENDING:
        width = 320;
        height = 200;
        graphics.loadPCXPalette("assets/CREDIT.GRP");
        break;
    default:
        width = 320;
        height = 200;
        break;
    }

    if (currentWidth == width && currentHeight == height) return;
    currentWidth = width;
    currentHeight = height;

    SDL_SetWindowSize(window, width * 2, height * 2);
    SDL_RenderSetLogicalSize(renderer, width, height);
    graphics.setResolution(width, height);
}

void Game::handleEvents() {
    SDL_Event event;

    controls &= AP_BUST;

    const Uint8* state_kb = SDL_GetKeyboardState(NULL);

    if (state == State::GAMEPLAY && health > 0) {
        if (state_kb[SDL_SCANCODE_UP]) controls |= AP_FORW;
        if (state_kb[SDL_SCANCODE_DOWN]) controls |= AP_BACK;
        if (state_kb[SDL_SCANCODE_LEFT]) controls |= AP_LEFT;
        if (state_kb[SDL_SCANCODE_RIGHT]) controls |= AP_RIGH;
        if (state_kb[SDL_SCANCODE_LCTRL] || state_kb[SDL_SCANCODE_RCTRL]) controls |= AP_FIRE;

        if (state_kb[SDL_SCANCODE_LALT] || state_kb[SDL_SCANCODE_RALT]) controls |= AP_SHFT;
        if (state_kb[SDL_SCANCODE_LSHIFT] || state_kb[SDL_SCANCODE_RSHIFT]) controls |= AP_ACCL;
    }

    while (SDL_PollEvent(&event)) {
        if (event.type == SDL_QUIT) running = false;
        if (event.type == SDL_KEYDOWN) {
            if (event.key.keysym.sym == SDLK_ESCAPE) {
                if (state == State::INTRO || state == State::ENDING) {
                    returnToTitle();
                }
                else running = false;
            }
            if (state == State::GAMEPLAY) {
                if (event.key.keysym.sym >= SDLK_1 && event.key.keysym.sym <= SDLK_9) {
                    weaponSlot = event.key.keysym.sym - SDLK_1;
                    audio.playSound(SoundId::UI_CLICK_A);
                }
                if (event.key.keysym.sym == SDLK_SPACE) {
                    if (health > 0) {
                        controls ^= AP_BUST;
                    }
                }
            }
            else if (state == State::INTRO) {
                if (event.key.keysym.sym == SDLK_SPACE || event.key.keysym.sym == SDLK_RETURN) {
                    if (introFliIdx + 1 < fliQueue.size()) {
                        advanceIntro();
                    }
                    else {
						returnToTitle();
                    }
                }
            }
            else if (state == State::TITLE) {
                if (event.key.keysym.sym == SDLK_UP) {
                    menuCursor = (menuCursor + 3) % 4;
                    audio.playSound(SoundId::UI_CLICK_A);
                }
                else if (event.key.keysym.sym == SDLK_DOWN) {
                    menuCursor = (menuCursor + 1) % 4;
                    audio.playSound(SoundId::UI_CLICK_A);
                }
                else if (event.key.keysym.sym == SDLK_RETURN || event.key.keysym.sym == SDLK_SPACE) {
                    audio.playSound(SoundId::UI_CONFIRM);
                    if (menuCursor == 0) {
                        transitionTo(State::GAMEPLAY);
                        loadMission(1);
                    }
                    else if (menuCursor == 3) {
                        running = false;
                    }
                }
            }
            else if (state == State::MISSION_CLEAR) {
                if (event.key.keysym.sym == SDLK_RETURN || event.key.keysym.sym == SDLK_SPACE) {
                    missionIndex++;
                    graphics.setBlueTint(false);
                    if (missionIndex > 15) {
                        transitionTo(State::ENDING);
                        audio.loadMusic("assets/FM015.MOD");
                        audio.playMusic();
                        ending_y = 220.0f;
                        lastEndingTick = SDL_GetTicks();
                    }
                    else {
                        transitionTo(State::GAMEPLAY);
                        loadMission(missionIndex);
                    }
                }
            }
            else if (state == State::ENDING) {
                if (event.key.keysym.sym == SDLK_RETURN || event.key.keysym.sym == SDLK_SPACE) {
                    returnToTitle();
                }
            }
        }
    }
}

void Game::fireWeapon() {
    if (camAlt > 133) {
        return;
    }

    if (ammo[weaponSlot] <= 0) {
        audio.playSound(SoundId::GUN_DRY);
        return;
    }
    ammo[weaponSlot]--;

    SoundId playedSound = SoundId::WEAPON_VULCAN;
    switch (weaponSlot) {
    case 0: playedSound = SoundId::WEAPON_VULCAN; break;
    case 1: playedSound = SoundId::MACHINEGUN; break;
    case 2:
    case 3: playedSound = SoundId::WEAPON_HEAVY; break;
    case 4:
    case 5:
    case 6: playedSound = SoundId::DROP; break;
    case 7:
    case 8: playedSound = SoundId::FLAMETHROWER; break;
    }
    audio.playSound(playedSound);

    if (weaponSlot == 0 || weaponSlot == 1) {
        int dmg = weapon_power[weaponSlot];
        int fire_d = 8;
        int max_dist = 8 * 24;

        int ray_x = camX;
        int ray_y = camY;

        int32_t step_x = 0;
        int32_t step_y = 0;
        calculateDeltaStep(static_cast<float>(headingInt), 8.0f, step_x, step_y);

        for (int i = 0; i < 24; ++i) {
            ray_x = (ray_x + step_x) & 0x07FFFFFF;
            ray_y = (ray_y + step_y) & 0x07FFFFFF;

            if (graphics.isCollision(ray_x, ray_y)) {
                spawnEffect(3, ray_x, ray_y);

                int tx = (ray_x >> 19) & 255;
                int ty = (ray_y >> 19) & 255;

                graphics.damageTile(tx, ty);

                max_dist = fire_d;
                break;
            }
            fire_d += 8;
        }

        Object* closest_enemy = nullptr;
        float min_dist = 99999.0f;

        for (auto& enemy : objects) {
            if (enemy.state <= 0 || enemy.health <= 0) continue;
            if ((enemy.typeId & 0x100) != 0) continue;

            int32_t dx = (enemy.x - camX) >> 16;
            int32_t dy = (enemy.y - camY) >> 16;
            float dist = std::sqrt(static_cast<float>(dx * dx + dy * dy));

            if (dist > 190.0f || dist > static_cast<float>(max_dist)) continue;

            int angle_diff = (enemy.angleToPlayer + 128 - headingInt) & 255;
            if (angle_diff >= 128) angle_diff -= 256;

            if (angle_diff >= -8 && angle_diff <= 8) {
                if (dist < min_dist || (dist == min_dist && (enemy.typeId >> 8) == 4)) {
                    min_dist = dist;
                    closest_enemy = &enemy;
                }
            }
        }

        if (closest_enemy != nullptr) {
            closest_enemy->health -= dmg;
            if (closest_enemy->health < 0) closest_enemy->health = 0;
            closest_enemy->speed = 0;

            uint32_t logicFrames = SDL_GetTicks() / 71;
            int rand_offset = static_cast<int>((logicFrames & 7) << 16);

            spawnEffect(0, closest_enemy->x + rand_offset, closest_enemy->y - rand_offset);
        }
    }
    else {
        uint8_t dir;
        if (weaponSlot == 4 || weaponSlot == 5 || weaponSlot == 6) {
            dir = 0;
            audio.playSound(SoundId::DROP);
        }
        else {
            dir = (weaponSlot < 7) ? 16 : 8;
        }

        if (weaponSlot == 5) {
            holo_x = camX;
            holo_y = camY;
            holo_stat = 1;
        }

        spawnProjectile(weaponSlot, camX, camY, dir, headingInt);

        if (weaponSlot == 2) {
            spawnProjectile(weaponSlot, camX, camY, dir, static_cast<uint8_t>((headingInt + 2) & 255));
            spawnProjectile(weaponSlot, camX, camY, dir, static_cast<uint8_t>((headingInt - 2) & 255));
        }
    }
}

void Game::update() {
    if (state == State::INTRO) {
        if (!fliPlayer.update()) advanceIntro();
    }
    else if (state == State::ENDING) {
        Uint32 currentTime = SDL_GetTicks();
        if (currentTime - lastEndingTick >= 57) {
            int ticksPassed = static_cast<int>((currentTime - lastEndingTick) / 57);
            ending_y -= static_cast<float>(ticksPassed);
            lastEndingTick += ticksPassed * 57;
        }

        if (ending_y < -500.0f) {
            returnToTitle();
        }
    }
    else if (state == State::GAMEPLAY) {

        float dt_ratio = 14.0f / 60.0f;

        if (!(controls & AP_SHFT)) {
            float turn_speed = (controls & AP_ACCL) ? 4.0f : 2.0f;
            turn_speed *= dt_ratio;

            if (controls & AP_LEFT) heading -= turn_speed;
            if (controls & AP_RIGH) heading += turn_speed;

            normalizeHeading(heading);
        }

        headingInt = static_cast<uint8_t>(heading);
        camYaw = static_cast<uint8_t>(192 - headingInt);

        if ((controls & AP_BUST) && boostFuel > 0) {
            if (fCamAlt < 203.0f) {
                fCamAlt += 5.0f * dt_ratio;
                if (fCamAlt > 203.0f) fCamAlt = 203.0f;
            }
        }
        else {
            if (fCamAlt > 128.0f) {
                fCamAlt -= 5.0f * dt_ratio;
                if (fCamAlt < 128.0f) fCamAlt = 128.0f;
            }
        }
        camAlt = static_cast<int>(fCamAlt);

        float move_speed = 0.0f;
        float move_angle_offset = 0.0f;

        if (controls & AP_FORW) {
            move_speed = ((controls & AP_BUST) && boostFuel > 0) ? (8.0f * dt_ratio) : (5.0f * dt_ratio);
        }
        else if (controls & AP_BACK) {
            move_speed = -5.0f * dt_ratio;
        }

        if (controls & AP_SHFT) {
            if (controls & AP_LEFT) {
                move_speed = 5.0f * dt_ratio;
                move_angle_offset = -64.0f;
            }
            else if (controls & AP_RIGH) {
                move_speed = 5.0f * dt_ratio;
                move_angle_offset = 64.0f;
            }
        }

        if (move_speed != 0.0f) {
            float move_angle_val = heading + move_angle_offset;
            normalizeHeading(move_angle_val);

            int32_t dx = 0;
            int32_t dy = 0;
            calculateDeltaStep(move_angle_val, move_speed, dx, dy);

            int32_t old_cam_x = camX;
            int32_t old_cam_y = camY;

            camX = (camX + dx) & 0x07FFFFFF;
            camY = (camY + dy) & 0x07FFFFFF;

            if (camAlt <= 133) {
                if (graphics.isCollision(camX, camY)) {
                    camX = old_cam_x;
                    camY = old_cam_y;

                    int tx = (camX >> 19) & 255;
                    int ty = (camY >> 19) & 255;

                    static const int off_x[16] = { 0,  1,  2,  2, 2, 2, 2, 1, 0, -1, -2, -2, -2, -2, -2, -1 };
                    static const int off_y[16] = { -2, -2, -2, -1, 0, 1, 2, 2, 2,  2,  2,  1,  0, -1, -2, -2 };

                    uint16_t ax = 0;
                    for (int i = 0; i < 16; ++i) {
                        int nx = (tx + off_x[i]) & 255;
                        int ny = (ty + off_y[i]) & 255;
                        if (graphics.isCollision(nx << 19, ny << 19)) {
                            ax |= (1 << i);
                        }
                    }

                    int cl = static_cast<int>(headingInt) >> 4;
                    int cl_right = (cl + 4) & 15;
                    uint16_t bx_right = 0xF800;
                    bx_right = static_cast<uint16_t>(((bx_right << cl_right) | (bx_right >> (16 - cl_right))) & 0xFFFF);

                    bool jump_rrr = false;
                    if ((ax & bx_right) != 0) {
                        jump_rrr = true;
                    }
                    else {
                        int cl_left = (cl + 5) & 15;
                        uint16_t bx_left = 0x001F;
                        bx_left = static_cast<uint16_t>(((bx_left << cl_left) | (bx_left >> (16 - cl_left))) & 0xFFFF);
                        if ((ax & bx_left) == 0) {
                            jump_rrr = true;
                        }
                    }

                    float turn_amt = 2.0f * dt_ratio;
                    bool is_moving_backward = (move_speed < 0.0f);

                    if (jump_rrr) {
                        if (is_moving_backward) heading -= turn_amt;
                        else                    heading += turn_amt;
                    }
                    else {
                        if (is_moving_backward) heading += turn_amt;
                        else                    heading -= turn_amt;
                    }

                    normalizeHeading(heading);

                    headingInt = static_cast<uint8_t>(heading);
                    camYaw = static_cast<uint8_t>(192 - headingInt);
                }
            }
        }

        updateShadowDir();

        Uint32 currentTime = SDL_GetTicks();

        while (currentTime - lastLogicTime >= 71) {
            lastLogicTime += 71;

            showJamNoise = false;

            for (auto& obj : objects) {
                obj.x &= 0x07FFFFFF;
                obj.y &= 0x07FFFFFF;
            }

            for (auto& obj : objects) {
                if (obj.state != 0) {
                    advanceObjectScript(obj);
                }
            }

            size_t numObjects = objects.size();
            for (size_t i = 0; i < numObjects; ++i) {
                if (objects[i].state > 0 || (objects[i].typeId & 0xFF00) == 0x0800) {
                    updateObjectAI(objects[i], i);
                }
            }

            flushPendingObjects();

            syncWorldCoords();

            static int fire_delay = 0;
            if (fire_delay > 0) fire_delay--;

            static const int FIRE_DELAY_TABLE[] = { 0, 0, 5, 6, 6, 6, 6, 8, 8 };
            if (controls & AP_FIRE) {
                if (fire_delay == 0) {
                    fireWeapon();
                    int wIdx = std::min(weaponSlot, 8);
                    fire_delay = FIRE_DELAY_TABLE[wIdx];
                }
            }

            flushPendingObjects();

            for (auto it = objects.begin(); it != objects.end(); ) {
                if (it->state == 0) {
                    it = objects.erase(it);
                }
                else {
                    ++it;
                }
            }

            updatePlayerAnimation();
            updateMissionGoals();

            if (health <= 0) {
                controls = 0;
                if (deathTriggered == 0) {
                    graphics.setRedTint(true);
                    spawnEffect(2, camX, camY);
                    audio.playSound(SoundId::EXPLOSION_LARGE);
                    deathDelay = 12;
                    deathTriggered = 1;
                }

                if (deathDelay > 0) {
                    deathDelay--;
                }
                else {
                    graphics.setRedTint(false);
                    returnToTitle();
                }
            }
            else {
                if (missionComplete == 0) {
                    if (goalAchieved[0] && goalAchieved[1] && goalAchieved[2]) {
                        missionComplete = 1;
                        graphics.setBlueTint(true);
                        transitionTo(State::MISSION_CLEAR);

                        int bonus_score = bonus[missionIndex - 1];
                        totalGold += bonus_score + goldEarned;
                    }
                }
            }
        }

        syncWorldCoords();
    }
}

void Game::updateMissionGoals() {
    static const int mission_type_array[] = { 1, 2, 3, 9, 8, 10, 5, 9, 8, 13, 5, 8, 1, 12, 7 };
    static const uint8_t mission_goal_table[][4] = {
        {1,0,0,0}, {1,0,1,0}, {0,1,0,0}, {0,1,1,0}, {1,1,0,0},
        {1,1,1,0}, {1,2,0,0}, {1,2,2,0}, {0,2,0,0}, {0,2,1,0},
        {0,2,2,0}, {2,0,0,0}, {1,0,2,0}, {0,1,2,0}
    };
    static const uint8_t kill_enemy_req[] = { 10,0,0,0,0, 0,20,0,0,0, 20,0,25,30,25 };

    int mIdx = missionIndex - 1;
    if (mIdx < 0 || mIdx >= 15) mIdx = 0;
    int typeIdx = mission_type_array[mIdx];

    targetLampState = -1;
    extractionLampState = -1;
    showTargetWarning = false;
    showReturnsMessage = false;

    uint32_t frames = SDL_GetTicks() / 71;

    if (goalAchieved[0] == 0) {
        if (mission_goal_table[typeIdx][0] == 0 || killCount >= kill_enemy_req[mIdx]) {
            goalAchieved[0] = 1;
        }
    }

    if (goalAchieved[0] == 1) {
        if (killTimer > 0) killTimer--;
    }

    if (goalAchieved[0] == 1 && goalAchieved[1] == 0) {
        if (mission_goal_table[typeIdx][1] == 0) {
            goalAchieved[1] = 1;
        }
        else if (mission_goal_table[typeIdx][1] == 1 || killTimer == 0) {
            targetLampState = 1;
            if ((frames & 3) != 0) showTargetWarning = true;

            bool blink = false;
            for (auto& obj : objects) {
                if (obj.state > 0 && obj.typeId == 0x800) {
                    int angle_diff = (obj.angleToPlayer + 128 - headingInt) & 255;
                    if (angle_diff >= 128) angle_diff -= 256;
                    if (angle_diff >= -4 && angle_diff <= 4) {
                        blink = true;
                        break;
                    }
                }
            }

            if (blink) {
                targetLampState = frames & 1;
            }

            if (targetsDestroyed >= targetCount) {
                goalAchieved[1] = 1;
            }
        }
    }

    if (goalAchieved[1] == 1) {
        if (retreatTimer > 0) retreatTimer--;
    }

    if (goalAchieved[1] == 1 && goalAchieved[2] == 0) {
        if (mission_goal_table[typeIdx][2] == 0) {
            goalAchieved[2] = 1;
        }
        else if (mission_goal_table[typeIdx][2] == 1 || retreatTimer == 0) {
            extractionLampState = 1;
            if ((frames & 7) != 0) showReturnsMessage = true;

            int angle_diff = (extractionAngle + 128 - headingInt) & 255;
            if (angle_diff >= 128) angle_diff -= 256;

            if (angle_diff >= -4 && angle_diff <= 4) {
                extractionLampState = frames & 1;
            }

            int dx = std::abs((camX - extractionX) >> 19);
            int dy = std::abs((camY - extractionY) >> 19);
            if (dx <= 2 && dy <= 2 && camAlt <= 133) {
                goalAchieved[2] = 1;
            }
        }
    }
}

void Game::updateShadowDir() {
    float angle = static_cast<float>((camYaw + 96) & 255) / 256.0f * 2.0f * static_cast<float>(M_PI);

    int32_t sine_val = static_cast<int32_t>(std::round(std::sin(angle) * 32768.0f));
    int32_t cosine_val = static_cast<int32_t>(std::round(std::cos(angle) * 32768.0f));

    int16_t ax_val = static_cast<int16_t>((5 * camAlt) / 128);

    int32_t v_dx = ax_val * cosine_val;
    int32_t v_dy = ax_val * sine_val;

    shadowDX = static_cast<int16_t>((v_dx * 2) >> 16);
    shadowDY = static_cast<int16_t>((v_dy * 2) >> 16);
}

void Game::updatePlayerAnimation() {
    if ((controls & AP_BUST) && boostFuel > 0) {
        if (playerFrame < 32) {
            playerFrame = 32;
            audio.playSound(SoundId::BOOSTER_START);
        }

        playerFrame++;
        if (playerFrame > 32 + 5) {
            playerFrame = 32 + 4;
        }

        boostFuel--;

        uint32_t logicFrames = SDL_GetTicks() / 71;
        if (logicFrames & 1) {
            audio.playSound(SoundId::BOOSTER_LOOP);
        }
    }
    else {
        if (camAlt > 128) {
            playerFrame--;
            if (playerFrame < 32) playerFrame = 33;
        }
        else {
            if (playerFrame >= 32) playerFrame = 0;

            int base = (std::min(weaponSlot, 2) + 1) * 8;
            int limit = base + 8;

            bool is_moving = (controls & (AP_FORW | AP_BACK | AP_LEFT | AP_RIGH)) != 0;

            if (is_moving) {
                playerFrame++;
                if (playerFrame == 8) playerFrame = 0;
                if (playerFrame == limit) playerFrame = base;
            }
            else {
                if ((playerFrame & 3) != 0) {
                    playerFrame++;
                    if (playerFrame == 8) playerFrame = 0;
                    if (playerFrame == limit) playerFrame = base;
                }
            }

            if ((controls & AP_FIRE) && ammo[weaponSlot] > 0) {
                if (playerFrame < 8) {
                    playerFrame += base;
                }
                else {
                    if (!is_moving) {
                        playerFrame -= base;
                    }
                }
            }
            else {
                if (playerFrame >= 8) {
                    playerFrame -= base;
                }
            }
        }
    }

    if (playerFrame > 44) playerFrame = 44;
    if (playerFrame < 0) playerFrame = 0;
}

void Game::render() {
    switch (state) {
    case State::INTRO:
        renderIntro();
        break;
    case State::TITLE:
        renderTitle();
        break;
    case State::GAMEPLAY:
    case State::MISSION_CLEAR:
        renderGameplay();
        break;
    case State::ENDING:
        renderEnding();
        break;
    }
    SDL_RenderPresent(renderer);
}

void Game::renderIntro() {
    fliPlayer.render();
}

void Game::renderTitle() {
    graphics.clearScreen(0xFF000000);

    if (titleSprite.width > 0) {
        graphics.drawSprite(titleSprite, 0, 0, false);
    }

    static const int menuY[] = { 46, 74, 102, 130 };
    for (int i = 0; i < 4; ++i) {
        int sprIdx = (i == static_cast<int>(menuCursor)) ? i + 4 : i;
        graphics.drawSprite(ResourceManager::getInstance().getMenuSprite(sprIdx), 64, menuY[i], true);
    }
    graphics.render();
}

void Game::renderEnding() {
    graphics.clearScreen(0xFF000000);

    const Sprite& creditSpr = ResourceManager::getInstance().getCreditSprite();
    if (creditSpr.width > 0) {
        graphics.drawSprite(creditSpr, 0, 0, false);
    }

    const auto& endingSprs = ResourceManager::getInstance().getEndingSprites();
    for (int i = 0; i < 15; ++i) {
        if (i < static_cast<int>(endingSprs.size())) {
            const Sprite& spr = endingSprs[i];
            int draw_x = 160 - spr.width / 2;
            int draw_y = static_cast<int>(ending_y) + (i * 32) - spr.height / 2;
            graphics.drawSprite(spr, draw_x, draw_y, true);
        }
    }
    graphics.render();
}

void Game::renderGameplay() {
    graphics.drawFloor(camX, camY, camAlt, camYaw);

    for (const auto& obj : objects) {
        if (obj.state == 0) continue;

        if ((obj.typeId & 0x100) == 0 && obj.typeId != 0x0800) {
            if (obj.spriteGroup == GRP_NONE) continue;
            const Sprite& sprite = ResourceManager::getInstance().getGroupSprite(obj.spriteGroup, obj.frameIdx);

            uint8_t vth = static_cast<uint8_t>(obj.heading + 64);
            int shadow_world_x = obj.x - (3 << 16);
            int shadow_world_y = obj.y + (3 << 16);

            graphics.drawSpriteRotatedShadow(sprite, shadow_world_x, shadow_world_y, obj.z, camX, camY, camAlt, camYaw, vth, obj.pivotX, obj.pivotY);
        }
    }

    for (const auto& obj : objects) {
        if (obj.state == 0) continue;
        if (obj.spriteGroup == GRP_NONE) continue;
        const Sprite& sprite = ResourceManager::getInstance().getGroupSprite(obj.spriteGroup, obj.frameIdx);

        if ((obj.typeId & 0x100) == 0 && obj.typeId != 0x0800) {
            uint8_t vth = static_cast<uint8_t>(obj.heading + 64);
            graphics.drawSpriteRotated(sprite, obj.x, obj.y, obj.z, camX, camY, camAlt, camYaw, vth, obj.pivotX, obj.pivotY);
        }
        else {
            graphics.drawSpritePerspective(sprite, obj.x, obj.y, obj.z, camX, camY, camAlt, camYaw);
        }
    }

    const auto& res = ResourceManager::getInstance();

    if (showJamNoise) {
        uint32_t frames = SDL_GetTicks() / 71;
        graphics.drawSprite(res.getNoiseSprite(frames & 3), 3, 3, true);
    }
    else {
        graphics.drawRadar(camX, camY, camYaw, objects);
        graphics.drawRect(32, 31, 1, 1, 84);
        graphics.drawRect(32, 33, 1, 1, 84);
        graphics.drawRect(31, 32, 1, 1, 84);
        graphics.drawRect(33, 32, 1, 1, 84);
    }

    graphics.drawSpriteShadowSC(res.getPlayerSprite(playerFrame), 179 + 13 + shadowDX, 186 + 14 + shadowDY, camAlt);
    graphics.drawSprite(res.getPlayerSprite(playerFrame), 179, 186, true);
    graphics.drawFloor2(camX, camY, camAlt, camYaw);

    graphics.drawSprite(res.getHudPanel(), 0, 0, true);
    graphics.drawSprite(res.getMfd1(), 11, 79, true);
    graphics.drawSprite(res.getMfd2(), 10, 130, true);
    graphics.drawSprite(res.getMfd3(), 13, 189, true);

    graphics.drawCompass(res.getCompass(), camYaw);

    if (targetLampState >= 0) {
        graphics.drawSprite(res.getLamp1Sprite(targetLampState), 6, 63, true);
    }
    if (showTargetWarning) {
        graphics.drawText5x5("TARGETS DETECTED", 100, 50, 80);
    }

    if (extractionLampState >= 0) {
        graphics.drawSprite(res.getLamp2Sprite(extractionLampState), 6, 63, true);
    }
    if (showReturnsMessage) {
        graphics.drawText5x5("RETURN POSITION DETECTED", 100, 50, 80);
    }

    if (health < 50) {
        uint8_t dangerColor = ((SDL_GetTicks() / 284) % 2) ? 92 : 88;
        graphics.drawText5x5("DANGER", 11 + 3, 79 + 10, dangerColor);
    }

    const char* weaponNames[] = { "V-T103", "MIG-M01", "ST-WK5", "SM-50G", "B-TSB", "H-MSB", "M-MSB", "FFAR", "HFAR" };
    int wIdx = weaponSlot;
    if (wIdx < 0) wIdx = 0;
    if (wIdx > 8) wIdx = 8;

    graphics.drawText5x5(weaponNames[wIdx], 10 + 1, 130 + 1, 9);
    graphics.drawText5x5("R:", 10 + 2, 130 + 1 + 6, 9);
    graphics.drawText5x5(std::to_string(ammo[wIdx]), 10 + 1 + 2 * 6, 130 + 1 + 6, 9);

    graphics.drawText5x5("BU:", 10 + 2, 130 + 1 + 6 * 4, 9);
    graphics.drawText5x5(std::to_string(boostFuel), 10 + 1 + 3 * 6, 130 + 1 + 6 * 4, 9);

    graphics.drawText5x5("PW:", 10 + 2, 130 + 1 + 6 * 5, 9);
    graphics.drawText5x5(std::to_string(health), 10 + 1 + 3 * 6, 130 + 1 + 6 * 5, 9);

    if (state == State::MISSION_CLEAR) {
        graphics.drawText5x5("MISSION ACCOMPLISHED", 100, 100, 80);
        graphics.drawText5x5("ENEMY DESTROYED", 100, 110, 80);
        graphics.drawText5x5("MISSION BONUS", 100, 120, 80);
        graphics.drawText5x5("TOTAL GOLDS", 100, 130, 80);
        graphics.drawText5x5("PRESS [ENTER] KEY", 100, 150, 80);

        graphics.drawText5x5(std::to_string(killCount), 200, 110, 80);
        graphics.drawText5x5("GOLDS", 230, 110, 80);
        graphics.drawText5x5(std::to_string(goldEarned), 270, 110, 80);

        int bonus_val = bonus[missionIndex - 1];
        graphics.drawText5x5(std::to_string(bonus_val), 200, 120, 80);
        graphics.drawText5x5(std::to_string(totalGold), 200, 130, 80);
    }

    graphics.render();
}

void Game::advanceObjectScript(Object& obj) {
    if (!obj.script || obj.scriptPos < 0) return;

    bool processNext = true;
    while (processNext) {
        int cmd = obj.script[obj.scriptPos];
        if (cmd == -1) break;

        if (cmd < 200) {
            obj.frameIdx = cmd;
            obj.scriptPos++;
            processNext = false;
        }
        else {
            switch (cmd) {
            case CMD_SFX:
                audio.playSound(static_cast<SoundId>(obj.script[obj.scriptPos + 1]));
                obj.scriptPos += 2;
                break;
            case CMD_KILL:
                obj.state = 0;
                if ((obj.typeId >> 8) == 6) {
                    holo_stat = 0;
                }
                processNext = false;
                break;
            case CMD_SPAWN_FLM:
                spawnEffect(obj.script[obj.scriptPos + 1], obj.x, obj.y);
                obj.scriptPos += 2;
                break;
            case CMD_GOTO:
                obj.scriptPos = obj.script[obj.scriptPos + 1];
                break;
            case CMD_JMP_IF_STAT:
                if (obj.state == obj.script[obj.scriptPos + 1]) {
                    obj.scriptPos = obj.script[obj.scriptPos + 2];
                }
                else {
                    obj.scriptPos += 3;
                }
                break;
            case CMD_JMP_IF_DEAD:
                if (obj.health <= 0) {
                    obj.scriptPos = obj.script[obj.scriptPos + 1];
                }
                else {
                    obj.scriptPos += 2;
                }
                break;
            default:
                obj.scriptPos++;
                break;
            }
        }
    }
}

void Game::applyObjectVelocity(Object& obj) {
    float velocity = obj.speed;
    if (velocity != 0) {
        int32_t dx = 0;
        int32_t dy = 0;
        calculateDeltaStep(static_cast<float>(obj.heading), velocity, dx, dy);

        obj.x = (obj.x + dx) & 0x07FFFFFF;
        obj.y = (obj.y + dy) & 0x07FFFFFF;
    }
}

void Game::updateObjectAI(Object& obj, size_t index) {
    if (obj.typeId != 0x508 && obj.fireCooldown > 0) {
        obj.fireCooldown--;
    }

    int typeCategory = obj.getCategory();

    switch (typeCategory) {
    case 5:
    case 6:
        updateProjectileAI(obj);
        break;
    case 0:
    case 2:
        updateInfantryAI(obj, index);
        break;
    case 3: {
        bool hit_player = false;

        if (obj.distToPlayer <= 300 && camAlt <= 133) {
            int dx = std::abs((obj.x - camX) >> 16);
            int dy = std::abs((obj.y - camY) >> 16);

            if (dx <= 10 && dy <= 10) {
                hit_player = true;
            }
        }

        if (hit_player) {
            health -= obj.health;
            if (health < 0) health = 0;
            showJamNoise = true;

            obj.state = 0;
            spawnEffect(1, obj.x, obj.y);
            return;
        }
        else if (graphics.isCollision(obj.x, obj.y)) {
            obj.state = 0;
            spawnEffect(1, obj.x, obj.y);
            return;
        }

        applyObjectVelocity(obj);
        break;
    }
    case 8:
        updateMissionTargetAI(obj);
        break;
    case 1:
        applyObjectVelocity(obj);
        break;
    case 4:
        updateTurretTopAI(obj, index);
        break;
    }
}

void Game::updateProjectileAI(Object& obj) {
    int wIdx = obj.typeId & 0xFF;
    int dmg = obj.health;

    if (obj.typeId == 0x504 || obj.typeId == 0x605) {
        int splash_range = (obj.typeId == 0x504) ? 50 : 80;

        if (obj.fireCooldown <= 0) {
            if (obj.typeId == 0x605) {
                holo_stat = 0;
            }
            for (auto& enemy : objects) {
                if (enemy.state <= 0 || enemy.health <= 0) continue;

                int enemyCat = enemy.getCategory();
                if (enemyCat == 1 || enemyCat == 3 || enemyCat == 5 || enemyCat == 6) continue;
                if (obj.typeId == 0x605 && enemy.typeId == 0x800) continue;

                int dx = std::abs((obj.x - enemy.x) >> 16);
                int dy = std::abs((obj.y - enemy.y) >> 16);

                if (dx <= splash_range && dy <= splash_range) {
                    enemy.health -= dmg;
                    if (enemy.health < 0) enemy.health = 0;
                    spawnEffect(2, enemy.x, enemy.y);
                }
            }

            audio.playSound(SoundId::EXPLOSION_SPLASH);
            obj.state = 0;
            spawnEffect(2, obj.x, obj.y);
        }
        return;
    }

    int range = weapon_range[wIdx];
    Object* hit_target = nullptr;
    float min_hit_dist = 99999.0f;

    for (auto& enemy : objects) {
        if (enemy.state <= 0 || enemy.health <= 0) continue;

        int enemyCat = enemy.getCategory();
        if (enemyCat == 1 || enemyCat == 3 || enemyCat == 5 || enemyCat == 6 || enemy.typeId == 0x800) continue;

        int dx = std::abs((obj.x - enemy.x) >> 16);
        int dy = std::abs((obj.y - enemy.y) >> 16);

        if (dx <= range && dy <= range) {
            float dist = std::sqrt(static_cast<float>(dx * dx + dy * dy));

            if (dist < min_hit_dist || (dist == min_hit_dist && enemyCat == 4)) {
                min_hit_dist = dist;
                hit_target = &enemy;
            }
        }
    }

    if (hit_target != nullptr) {
        hit_target->health -= dmg;
        if (hit_target->health < 0) hit_target->health = 0;

        spawnEffect(1, hit_target->x, hit_target->y);
        audio.playSound(SoundId::EXPLOSION_SPLASH);

        obj.state = 0;
        spawnEffect(2, obj.x, obj.y);
        return;
    }
    else if (graphics.isCollision(obj.x, obj.y)) {
        obj.state = 0;
        spawnEffect(1, obj.x, obj.y);

        int tx = (obj.x >> 19) & 255;
        int ty = (obj.y >> 19) & 255;
        graphics.damageTile(tx, ty);

        return;
    }

    if (obj.typeId == 0x508) {
        Object* closest_enemy = nullptr;
        float min_dist = 999999.0f;

        for (auto& enemy : objects) {
            if (enemy.state <= 0 || enemy.health <= 0) continue;

            int enemyCat = enemy.getCategory();
            if (enemyCat == 1 || enemyCat == 3 || enemyCat == 5 || enemyCat == 6 || enemy.typeId == 0x800) continue;

            int32_t pdx = (enemy.x - camX) >> 16;
            int32_t pdy = (enemy.y - camY) >> 16;
            float dist = std::sqrt(static_cast<float>(pdx * pdx + pdy * pdy));

            if (dist < min_dist) {
                min_dist = dist;
                closest_enemy = &enemy;
            }
        }

        if (closest_enemy != nullptr) {
            int32_t dx = closest_enemy->x - obj.x;
            int32_t dy = closest_enemy->y - obj.y;

            uint8_t target_theta = calculateAngle(static_cast<float>(dx), static_cast<float>(dy));

            int diff = target_theta - obj.heading;

            if (diff >= 128) diff -= 256;
            else if (diff < -128) diff += 256;

            if (diff > 0) {
                obj.heading = static_cast<uint8_t>(obj.heading + std::min(4, diff));
            }
            else if (diff < 0) {
                obj.heading = static_cast<uint8_t>(obj.heading - std::min(4, -diff));
            }
        }
    }

    applyObjectVelocity(obj);
}

void Game::updateInfantryAI(Object& obj, size_t index) {
    if (obj.state > 0) {
        if (obj.health <= 0) {
            obj.state = -1;
            killCount++;

            int subtype = (obj.typeId & 0xFF) - 1;
            if (subtype >= 0 && subtype < 10) {
                goldEarned += enemy_gold[subtype];
            }

            spawnLargeExplosionAndDebris(obj.x, obj.y);
            return;
        }

        int typeCategory = obj.getCategory();
        if (typeCategory == 2) {
            bool hasTop = false;

            if (index + 1 < objects.size()) {
                Object& nextObj = objects[index + 1];
                if (nextObj.getCategory() == 4 && nextObj.state > 0) {
                    hasTop = true;
                }
            }

            if (!hasTop) {
                uint32_t frames = SDL_GetTicks() / 71;
                if ((frames % 4) == 0) {
                    spawnEffect(4, obj.x, obj.y);
                }
            }
        }

        int distance = obj.distToPlayer;
        uint8_t target_theta = obj.angleToPlayer;

        int dspeed = obj.defaultSpeed;
        int dth = obj.turnSpeed;

        int32_t oldX = obj.x;
        int32_t oldY = obj.y;

        if (distance > 300) {
            obj.speed = 0;
            obj.state = 1;
        }
        else {
            obj.speed = dspeed;

            if (distance <= 60 && obj.health > 30) {
                obj.speed = 0;
            }

            if (obj.speed > 0) {
                applyObjectVelocity(obj);
            }

            if (graphics.isCollision(obj.x, obj.y)) {
                obj.x = oldX;
                obj.y = oldY;
                obj.headingStep = static_cast<uint8_t>(dth);
                obj.heading = static_cast<uint8_t>(obj.heading + obj.headingStep);
                obj.state = 1;
            }
            else {
                uint8_t u_diff = target_theta - obj.heading;

                if (u_diff != 0) {
                    if (u_diff < 128) {
                        obj.state = (u_diff <= 4) ? 2 : 1;
                        int dmth_val = dth;
                        if (obj.health < 30) {
                            dmth_val = -dth;
                        }
                        obj.headingStep = static_cast<uint8_t>(dmth_val);
                        obj.heading = static_cast<uint8_t>(obj.heading + obj.headingStep);
                    }
                    else {
                        obj.state = (u_diff >= 252) ? 2 : 1;
                        int dmth_val = -dth;
                        if (obj.health < 30) {
                            dmth_val = dth;
                        }
                        obj.headingStep = static_cast<uint8_t>(dmth_val);
                        obj.heading = static_cast<uint8_t>(obj.heading + obj.headingStep);
                    }
                }
                else {
                    obj.state = 2;
                }

                if (obj.state == 2 && typeCategory == 0) {
                    int subtype = obj.typeId & 0xFF;

                    if (subtype == 2 || subtype == 4) {
                        if (obj.fireCooldown == 0) {
                            spawnProjectile(7, obj.x, obj.y, 8, obj.angleToPlayer, true);
                            audio.playSound(SoundId::FLAMETHROWER);
                            obj.fireCooldown = 16;
                        }
                    }
                    else if (subtype == 3) {
                        if (obj.fireCooldown == 0) {
                            spawnProjectile(2, obj.x, obj.y, 16, obj.angleToPlayer, true);
                            spawnProjectile(2, obj.x, obj.y, 16, static_cast<uint8_t>((obj.angleToPlayer + 2) & 255), true);
                            spawnProjectile(2, obj.x, obj.y, 16, static_cast<uint8_t>((obj.angleToPlayer - 2) & 255), true);
                            audio.playSound(SoundId::WEAPON_HEAVY);
                            obj.fireCooldown = 8;
                        }
                    }
                    else {
                        if (camAlt <= 133 && obj.distToPlayer <= 180) {
                            audio.playSound(SoundId::MACHINEGUN);

                            int ray_x = obj.x;
                            int ray_y = obj.y;

                            int32_t step_x = 0;
                            int32_t step_y = 0;
                            calculateDeltaStep(static_cast<float>(obj.heading), 8.0f, step_x, step_y);

                            int steps = obj.distToPlayer / 8;
                            for (int i = 0; i < steps; ++i) {
                                ray_x = (ray_x + step_x) & 0x07FFFFFF;
                                ray_y = (ray_y + step_y) & 0x07FFFFFF;
                                if (graphics.isCollision(ray_x, ray_y)) {
                                    break;
                                }
                            }

                            spawnEffect(0, ray_x, ray_y);
                            audio.playSound(SoundId::EXPLOSION_SPLASH);

                            int dx = std::abs((ray_x - camX) >> 16);
                            int dy = std::abs((ray_y - camY) >> 16);
                            if (dx <= 10 && dy <= 10) {
                                health -= 1;
                                if (health < 0) health = 0;
                                showJamNoise = true;
                            }
                        }
                    }
                }
            }
        }
        obj.drawAngle = static_cast<uint8_t>(obj.heading + 64);
    }
}

void Game::updateTurretTopAI(Object& obj, size_t index) {
    if (obj.health <= 0) {
        if (obj.state != 0) {
            obj.state = 0;
            spawnLargeExplosionAndDebris(obj.x, obj.y);
        }
        return;
    }

    Object* myBase = nullptr;
    if (index > 0) {
        Object& prevObj = objects[index - 1];
        if (prevObj.getCategory() == 2 && prevObj.state > 0) {
            myBase = &prevObj;
        }
    }

    if (myBase) {
        obj.x = myBase->x;
        obj.y = myBase->y;
    }
    else {
        obj.health = 0;
        return;
    }

    uint8_t target_theta = obj.angleToPlayer;
    int distance = obj.distToPlayer;

    obj.heading = static_cast<uint8_t>(target_theta + 128);
    obj.drawAngle = static_cast<uint8_t>(obj.heading + 64);

    uint32_t frames = SDL_GetTicks() / 71;
    if ((frames & 15) == 0 && distance <= 180) {
        audio.playSound(SoundId::CANNON);
        spawnProjectile(3, obj.x, obj.y, 16, target_theta, true);
    }
}

void Game::updateMissionTargetAI(Object& obj) {
    if (obj.health > 0) {
        applyObjectVelocity(obj);
    }
    else {
        if (obj.state == -1) {
            uint32_t frames = SDL_GetTicks() / 71;
            if ((frames & 1) == 0) {
                spawnDebris(2, obj.x, obj.y, frames & 255, 3);
            }
        }
        else {
            obj.state = -1;
            targetsDestroyed++;

            audio.playSound(SoundId::EXPLOSION_SPLASH);
            audio.playSound(SoundId::EXPLOSION_SMALL);
            damageGroundArea(obj.x, obj.y);
        }
    }
}

void Game::spawnProjectile(int weapon_id, int x, int y, uint8_t dir, uint8_t th, bool is_enemy) {
    Object bullet;
    std::memset(&bullet, 0, sizeof(bullet));
    bullet.state = 1;
    bullet.x = x;
    bullet.y = y;
    bullet.z = 0;
    bullet.heading = th;
    bullet.drawAngle = th;
    bullet.speed = dir;

    uint16_t cat = is_enemy ? 0x300 : 0x500;
    if (!is_enemy && weapon_id == 5) {
        cat = 0x600;
    }
    bullet.typeId = cat | weapon_id;

    if (weapon_id == 2 || weapon_id == 3) {
        bullet.health = (weapon_id == 2) ? 4 : 25;
        bullet.spriteGroup = GRP_FLM1;
        bullet.script = blt1_crs_data;
    }
    else if (weapon_id == 4) { // Time Bomb (504h)
        bullet.health = 100;
        bullet.fireCooldown = 60;
        bullet.spriteGroup = GRP_TBOMB;
        bullet.script = blt2_crs_data;
    }
    else if (weapon_id == 5) { // Hologram Mine (605h)
        bullet.health = 80;
        bullet.fireCooldown = 60;
        bullet.spriteGroup = GRP_HOLO;
        bullet.script = blt2_crs_data;
        bullet.pivotX = 16;
        bullet.pivotY = 16;
    }
    else if (weapon_id == 6) { // Mine (506h)
        bullet.health = 150;
        bullet.fireCooldown = 0;
        bullet.spriteGroup = GRP_MINE;
        bullet.script = blt2_crs_data;
    }
    else if (weapon_id >= 7) {
        bullet.health = (weapon_id == 7) ? 80 : 60;
        bullet.spriteGroup = GRP_FLM1;
        bullet.script = missile1_crs_data;
    }
    else {
        bullet.health = weapon_power[weapon_id];
        bullet.spriteGroup = GRP_FLM1;
        bullet.script = blt1_crs_data;
    }

    bullet.scriptPos = 0;
    pendingObjects.push_back(bullet);
}

void Game::syncWorldCoords() {
    camX &= 0x07FFFFFF;
    camY &= 0x07FFFFFF;

    const int32_t target_x = holo_stat == 0 ? camX : holo_x;
    const int32_t target_y = holo_stat == 0 ? camY : holo_y;

    for (auto& obj : objects) {
        if (obj.state == 0) continue;

        obj.x &= 0x07FFFFFF;
        obj.y &= 0x07FFFFFF;

        wrapCoordinate(camX, obj.x);
        wrapCoordinate(camY, obj.y);

        if ((obj.typeId & 0x100) == 0 && obj.typeId != 0x0800) {
            int32_t dx = (target_x - obj.x) >> 16;
            int32_t dy = (target_y - obj.y) >> 16;

            float dist_f = std::sqrt(static_cast<float>(dx * dx + dy * dy));
            obj.distToPlayer = static_cast<uint16_t>(dist_f);

            if (dx != 0 || dy != 0) {
                obj.angleToPlayer = calculateAngle(static_cast<float>(dx), static_cast<float>(dy));
            }
        }
    }

    extractionX &= 0x07FFFFFF;
    extractionY &= 0x07FFFFFF;

    wrapCoordinate(camX, extractionX);
    wrapCoordinate(camY, extractionY);

    int32_t dx_r = (camX - extractionX) >> 16;
    int32_t dy_r = (camY - extractionY) >> 16;
    extractionDist = static_cast<int>(std::sqrt(static_cast<float>(dx_r * dx_r + dy_r * dy_r)));

    if (dx_r != 0 || dy_r != 0) {
        extractionAngle = calculateAngle(static_cast<float>(dx_r), static_cast<float>(dy_r));
    }
}

void Game::spawnEffect(int type, int x, int y) {
    Object flm;
    std::memset(&flm, 0, sizeof(flm));
    flm.state = 1;
    flm.x = x;
    flm.y = y;
    flm.z = 0;
    flm.typeId = 0x100 | type;

    if (type == 0) { flm.spriteGroup = GRP_FLM1;  flm.script = flm1_crs_data; }
    else if (type == 1) { flm.spriteGroup = GRP_FLM2;  flm.script = flm2_crs_data; }
    else if (type == 2) { flm.spriteGroup = GRP_FLM3;  flm.script = flm3_crs_data; }
    else if (type == 3) { flm.spriteGroup = GRP_FLM4;  flm.script = flm4_crs_data; }
    else if (type == 4) { flm.spriteGroup = GRP_DUST1; flm.script = dust1_crs_data; }
    else { flm.spriteGroup = GRP_FLM1;  flm.script = flm1_crs_data; }

    flm.scriptPos = 0;
    pendingObjects.push_back(flm);
}

void Game::spawnDebris(int type, int x, int y, uint8_t th, uint16_t dir) {
    Object chip;
    std::memset(&chip, 0, sizeof(chip));
    chip.state = 1;
    chip.x = x;
    chip.y = y;
    chip.z = 0;
    chip.heading = th;
    chip.speed = dir;

    if (type == 0) {
        chip.typeId = 0x104;
        chip.spriteGroup = GRP_CHIP1;
        chip.script = chip1_crs_data;
    }
    else if (type == 1) {
        chip.typeId = 0x105;
        chip.spriteGroup = GRP_CHIP2;
        chip.script = chip2_crs_data;
    }
    else if (type == 2) {
        chip.typeId = 0x106;
        chip.spriteGroup = GRP_SMK1;
        chip.script = smk1_crs_data;
    }

    chip.scriptPos = 0;
    pendingObjects.push_back(chip);
}

void Game::damageGroundArea(int32_t objX, int32_t objY) {
    int32_t mx = ((objX >> 19) & 255) - 5;
    int32_t my = ((objY >> 19) & 255) - 5;

    for (int i = 0; i < 10; ++i) {
        int32_t cy = (my + i) & 255;
        for (int j = 0; j < 10; ++j) {
            int32_t cx = (mx + j) & 255;
            graphics.damageTile(cx, cy);
        }
    }
}

void Game::buildEnemyObjects() {
    std::vector<Object> finalObjs;

    for (auto& obj : objects) {
        int type = obj.typeId;
        obj.frameIdx = 0;

        if (type == 1) {
            obj.typeId = type;
            obj.spriteGroup = GRP_ENEMY1;
            obj.script = walk1_crs_data;
            obj.health = 140;
            obj.heading = 64;
            obj.headingStep = 3;
            obj.turnSpeed = 3;
            obj.speed = 4;
            obj.defaultSpeed = 4;
            obj.pivotX = 16;
            obj.pivotY = 16;
            obj.state = 1;
        }
        else if (type == 2) {
            obj.typeId = type;
            obj.spriteGroup = GRP_ENEMY1;
            obj.script = walk1_crs_data;
            obj.health = 140;
            obj.heading = 64;
            obj.headingStep = 4;
            obj.turnSpeed = 4;
            obj.speed = 3;
            obj.defaultSpeed = 3;
            obj.pivotX = 16;
            obj.pivotY = 16;
            obj.state = 1;
        }
        else if (type == 3 || type == 4) {
            obj.typeId = type;
            obj.spriteGroup = GRP_ENEMY3;
            obj.script = walk3_crs_data;
            obj.health = 150;
            obj.heading = 32;
            obj.headingStep = 2;
            obj.turnSpeed = 2;
            obj.speed = 3;
            obj.defaultSpeed = 3;
            obj.pivotX = 16;
            obj.pivotY = 16;
            obj.state = 1;
        }
        else if (type == 5) {
            obj.typeId = type;
            obj.spriteGroup = GRP_ENEMY2;
            obj.script = walk1_crs_data;
            obj.health = 100;
            obj.heading = 32;
            obj.headingStep = 3;
            obj.turnSpeed = 3;
            obj.speed = 3;
            obj.defaultSpeed = 3;
            obj.pivotX = 16;
            obj.pivotY = 16;
            obj.state = 1;
        }
        else if (type == 6) {
            obj.typeId = type;
            obj.spriteGroup = GRP_ENEMY2;
            obj.script = walk1_crs_data;
            obj.health = 120;
            obj.heading = 32;
            obj.headingStep = 3;
            obj.turnSpeed = 3;
            obj.speed = 3;
            obj.defaultSpeed = 3;
            obj.pivotX = 16;
            obj.pivotY = 16;
            obj.state = 1;
        }
        else if (type == 7) {
            obj.typeId = 0x007;
            obj.spriteGroup = GRP_ENEMY4;
            obj.script = walk4_crs_data;
            obj.health = 100;
            obj.heading = 32;
            obj.headingStep = 4;
            obj.turnSpeed = 4;
            obj.speed = 5;
            obj.defaultSpeed = 5;
            obj.pivotX = 16;
            obj.pivotY = 16;
            obj.state = 1;
        }
        else if (type >= 8 && type <= 10) {
            obj.typeId = 0x200 | type;
            if (type == 8)  obj.spriteGroup = GRP_TANK1;
            if (type == 9)  obj.spriteGroup = GRP_TANK2;
            if (type == 10) obj.spriteGroup = GRP_TANK3;

            obj.script = tank_crs_data;
            obj.health = (type == 10) ? 90 : 80;
            obj.heading = 0;
            obj.headingStep = 2;
            obj.turnSpeed = 2;
            obj.speed = (type == 10) ? 1 : 2;
            obj.defaultSpeed = (type == 10) ? 1 : 2;
            obj.drawAngle = 64;
            obj.pivotX = 16;
            obj.pivotY = (type == 10) ? 9 : 15;
            obj.state = 1;
            obj.scriptPos = 0;

            finalObjs.push_back(obj);

            Object top = obj;
            top.typeId = 0x400 | type;

            if (type == 8)  top.spriteGroup = GRP_TTOP1;
            if (type == 9)  top.spriteGroup = GRP_TTOP2;
            if (type == 10) top.spriteGroup = GRP_TTOP3;

            top.script = nullptr;
            top.health = 70;
            top.heading = 0;
            top.headingStep = 2;
            top.turnSpeed = 2;
            top.speed = 0;
            top.defaultSpeed = 0;
            top.pivotX = 16;
            top.pivotY = (type == 10) ? 15 : 10;
            top.state = 1;

            finalObjs.push_back(top);
            continue;
        }
        else if (type == 20) {
            obj.typeId = 0x0800;
            obj.spriteGroup = GRP_NONE;
            obj.script = nullptr;
            obj.health = 1;
            obj.speed = 0;
            obj.defaultSpeed = 0;
            obj.heading = 0;
            obj.headingStep = 0;
            obj.turnSpeed = 0;
            obj.pivotX = 0;
            obj.pivotY = 0;
            obj.state = 1;
        }
        else {
            obj.typeId = 0x001;
            obj.spriteGroup = GRP_ENEMY1;
            obj.script = walk1_crs_data;
            obj.health = 100;
            obj.heading = 64;
            obj.headingStep = 3;
            obj.turnSpeed = 3;
            obj.speed = 4;
            obj.defaultSpeed = 4;
            obj.pivotX = 16;
            obj.pivotY = 16;
            obj.state = 1;
        }

        obj.scriptPos = 0;
        obj.drawAngle = static_cast<uint8_t>(obj.heading + 64);
        finalObjs.push_back(obj);
    }

    objects = finalObjs;
}

void Game::returnToTitle() {
    transitionTo(State::TITLE);
    introFliIdx = 0;
    menuCursor = 0;
    audio.loadMusic("assets/FM017.MOD");
    audio.playMusic();
}

void Game::normalizeHeading(float& angle) {
    if (angle < 0.0f) angle += 256.0f;
    if (angle >= 256.0f) angle -= 256.0f;
}

void Game::spawnLargeExplosionAndDebris(int32_t x, int32_t y) {
    audio.playSound(SoundId::EXPLOSION_LARGE);
    spawnEffect(1, x, y);

    spawnDebris(0, x, y, 0, 5);
    spawnDebris(1, x, y, 23, 4);
    spawnDebris(0, x, y, 120, 5);
    spawnDebris(1, x, y, 190, 5);
}

void Game::wrapCoordinate(int camCoord, int& objCoord) {
    constexpr int32_t MAP_SIZE = 2048 << 16;
    constexpr int32_t THRESHOLD = 500 << 16;
    constexpr int32_t UPPER_BOUND = MAP_SIZE - THRESHOLD;

    if (camCoord >= UPPER_BOUND) {
        if (objCoord < THRESHOLD) objCoord += MAP_SIZE;
    }
    else if (camCoord < THRESHOLD) {
        if (objCoord >= UPPER_BOUND) objCoord -= MAP_SIZE;
    }
}

uint8_t Game::calculateAngle(float dx, float dy) {
    float angle_rad = std::atan2(dy, dx);
    if (angle_rad < 0.0f) {
        angle_rad += 2.0f * static_cast<float>(M_PI);
    }
    return static_cast<uint8_t>(angle_rad * 256.0f / (2.0f * static_cast<float>(M_PI)));
}

void Game::calculateDeltaStep(float headingVal, float speed, int32_t& outDx, int32_t& outDy) {
    float angle_rad = (headingVal / 256.0f) * 2.0f * static_cast<float>(M_PI);
    float sined = std::sin(angle_rad);
    float cosined = std::cos(angle_rad);
    outDx = static_cast<int32_t>(speed * cosined * 65536.0f);
    outDy = static_cast<int32_t>(speed * sined * 65536.0f);
}

void Game::flushPendingObjects() {
    if (!pendingObjects.empty()) {
        objects.insert(objects.end(), pendingObjects.begin(), pendingObjects.end());
        pendingObjects.clear();
    }
}
