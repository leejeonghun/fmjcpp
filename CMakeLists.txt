cmake_minimum_required(VERSION 3.28)
project(fmjcpp)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

if(NOT EMSCRIPTEN)
    find_package(SDL2 CONFIG REQUIRED)
    find_package(SDL2_mixer CONFIG REQUIRED)

    include_directories(${SDL2_INCLUDE_DIRS} ${SDL2_MIXER_INCLUDE_DIRS})
endif()

set(SOURCES
    src/main.cpp
    src/game.cpp
    src/graphics.cpp
    src/audio.cpp
    src/fli.cpp
    src/resource_manager.cpp
    src/mission.cpp
)

add_executable(fmjcpp ${SOURCES})

if(MSVC)
    target_compile_definitions(fmjcpp PRIVATE _CRT_SECURE_NO_WARNINGS)
    target_compile_definitions(fmjcpp PRIVATE _USE_MATH_DEFINES)
    target_compile_options(fmjcpp PRIVATE /utf-8 /MP)

    target_sources(fmjcpp PRIVATE win32/fmjcpp.rc)
    set_target_properties(fmjcpp PROPERTIES 
        WIN32_EXECUTABLE "$<CONFIG:Release>"
        VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
    )
endif()

if(EMSCRIPTEN)
    target_compile_options(fmjcpp PRIVATE -sUSE_SDL=2 -sUSE_SDL_MIXER=2)
    target_link_options(fmjcpp PRIVATE
        -sUSE_SDL=2
        -sUSE_SDL_MIXER=2
        -sSDL2_MIXER_FORMATS=["mod"]
        -sWASM=1
        -sALLOW_MEMORY_GROWTH=1
        -sASYNCIFY=1
        --preload-file ${CMAKE_SOURCE_DIR}/assets@/assets
    )
    set_target_properties(fmjcpp PROPERTIES
        OUTPUT_NAME "index"
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/docs"
        SUFFIX ".html"
    )
else()
    target_link_libraries(fmjcpp PRIVATE
        $<TARGET_NAME_IF_EXISTS:SDL2::SDL2main>
        $<IF:$<TARGET_EXISTS:SDL2::SDL2>,SDL2::SDL2,SDL2::SDL2-static>
    )
    target_link_libraries(fmjcpp PRIVATE 
        $<IF:$<TARGET_EXISTS:SDL2_mixer::SDL2_mixer>,SDL2_mixer::SDL2_mixer,SDL2_mixer::SDL2_mixer-static>
    )
endif()
