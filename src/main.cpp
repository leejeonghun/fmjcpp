#include <SDL2/SDL.h>
#include <iostream>
#include "game.h"

int main(int argc, char* argv[]) {
    // Prevent unused parameter compiler warnings
    (void)argc;
    (void)argv;

    // Initialize core SDL subsystems (Video, Audio, and Timer)
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_TIMER) < 0) {
        std::cerr << "SDL could not initialize! SDL_Error: " << SDL_GetError() << std::endl;
        return 1;
    }

    // Open initial window targeting 320x200 resolution scaled by 2x (640x400)
    SDL_Window* window = SDL_CreateWindow("Full Metal Jacket",
        SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
        320 * 2, 200 * 2, SDL_WINDOW_SHOWN);
    if (!window) {
        std::cerr << "Window could not be created! SDL_Error: " << SDL_GetError() << std::endl;
        SDL_Quit();
        return 1;
    }

    // Create hardware-accelerated renderer with present vertical synchronization enabled
    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (!renderer) {
        std::cerr << "Renderer could not be created! SDL_Error: " << SDL_GetError() << std::endl;
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    // Scope block to ensure Game object destructs before terminating the subsystems
    {
        Game game(window, renderer);
        game.run();
    }

    // Graceful release of all assigned system resources
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}
