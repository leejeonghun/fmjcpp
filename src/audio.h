#ifndef AUDIO_H
#define AUDIO_H

#include <string>
#include <vector>
#include <SDL2/SDL_mixer.h>

// Strongly-typed sound effect identifiers corresponding to the original game's asset list.
enum class SoundId {
    WEAPON_VULCAN     = 0,  // FMJ01.WAV
    WEAPON_HIT        = 1,  // FMJ02.WAV
    EXPLOSION_SPLASH  = 2,  // FMJ03.WAV
    UI_CLICK_A        = 3,  // CLICK3.WAV
    UI_CLICK_B        = 4,  // CLICK10.WAV
    UI_CONFIRM        = 5,  // SFX01.WAV
    SIREN             = 6,  // SIREN1.WAV
    BAND              = 7,  // BAND.WAV
    SLIDE             = 8,  // SLD.WAV
    GUN_DRY           = 9,  // GUNDRY.WAV
    EXPLOSION_SMALL   = 10, // EXPLO1.WAV
    EXPLOSION_LARGE   = 11, // EXPLO2.WAV
    WEAPON_HEAVY      = 12, // SGUNSH.WAV
    WEAPON_ACTION     = 13, // SGUNAC.WAV
    CANNON            = 14, // CANNON.WAV
    FLAMETHROWER      = 15, // FTHROW.WAV
    MACHINEGUN        = 16, // MCHGUN.WAV
    DROP              = 17, // DROP.WAV
    BOOSTER_START     = 18, // BUSTON.WAV
    BOOSTER_LOOP      = 19, // BUST.WAV
    COUNT             = 20
};

class Audio {
public:
    Audio();
    ~Audio();

    // Background music (BGM) management
    bool loadMusic(const std::string& filename);
    void playMusic(int loops = -1);
    void stopMusic();

    // Sound effect (SFX) management with strict type safety
    bool loadSound(SoundId id, const std::string& filename);
    void playSound(SoundId id);

private:
    static constexpr int SFX_GROUP_ID = 1;
    static constexpr int MAX_AUDIO_CHANNELS = 8;
    static constexpr int MAX_SOUND_EFFECTS = 20;

    Mix_Music* activeMusic;
    std::vector<Mix_Chunk*> soundChunks;
};

#endif