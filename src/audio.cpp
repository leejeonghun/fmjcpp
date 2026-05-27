#include "audio.h"
#include <iostream>

Audio::Audio() : activeMusic(nullptr) {
    // Initialize SDL_mixer for tracker music modules (.MOD format)
    const int mixFlags = MIX_INIT_MOD;
    if ((Mix_Init(mixFlags) & mixFlags) != mixFlags) {
        std::cerr << "Warning: SDL_mixer MOD support failed to initialize: " << Mix_GetError() << std::endl;
    }

    // Open audio system with standardized sample rates and buffer sizes
    if (Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 2048) < 0) {
        std::cerr << "Warning: Mix_OpenAudio failed: " << Mix_GetError() << std::endl;
    }

    // Allocate audio mixing channels and categorize them into a group
    Mix_AllocateChannels(MAX_AUDIO_CHANNELS);
    for (int i = 0; i < MAX_AUDIO_CHANNELS; ++i) {
        Mix_GroupChannel(i, SFX_GROUP_ID);
    }

    soundChunks.resize(MAX_SOUND_EFFECTS, nullptr);
}

Audio::~Audio() {
    stopMusic();
    for (Mix_Chunk* chunk : soundChunks) {
        if (chunk) {
            Mix_FreeChunk(chunk);
        }
    }
    Mix_CloseAudio();
    Mix_Quit();
}

bool Audio::loadMusic(const std::string& filename) {
    stopMusic();
    activeMusic = Mix_LoadMUS(filename.c_str());

    if (!activeMusic) {
        std::cerr << "Failed to load music (" << filename << "): " << Mix_GetError() << std::endl;
        return false;
    }
    return true;
}

void Audio::playMusic(int loops) {
    if (activeMusic) {
        if (Mix_PlayMusic(activeMusic, loops) == -1) {
            std::cerr << "Failed to play music: " << Mix_GetError() << std::endl;
        }
    }
}

void Audio::stopMusic() {
    if (activeMusic) {
        Mix_HaltMusic();
        Mix_FreeMusic(activeMusic);
        activeMusic = nullptr;
    }
}

bool Audio::loadSound(SoundId id, const std::string& filename) {
    const int index = static_cast<int>(id);
    if (index < 0 || index >= static_cast<int>(soundChunks.size())) {
        return false;
    }
    
    if (soundChunks[index]) {
        Mix_FreeChunk(soundChunks[index]);
    }

    soundChunks[index] = Mix_LoadWAV(filename.c_str());
    if (!soundChunks[index]) {
        std::cerr << "Failed to load sound (" << filename << "): " << Mix_GetError() << std::endl;
        return false;
    }
    return true;
}

void Audio::playSound(SoundId id) {
    const int index = static_cast<int>(id);
    if (index >= 0 && index < static_cast<int>(soundChunks.size()) && soundChunks[index]) {
        // Try to find any currently free channel to play the sound effect
        int channel = Mix_PlayChannel(-1, soundChunks[index], 0);

        // Fallback: If all channels are busy, steal the oldest active channel in our SFX group
        if (channel == -1) {
            int oldestChannel = Mix_GroupOldest(SFX_GROUP_ID);
            if (oldestChannel != -1) {
                Mix_PlayChannel(oldestChannel, soundChunks[index], 0);
            }
        }
    }
}