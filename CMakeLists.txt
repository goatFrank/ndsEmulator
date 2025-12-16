cmake_minimum_required(VERSION 3.16)
project(ndsEmulator C)

# C11 standard
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Optimization flags
if(CMAKE_BUILD_TYPE STREQUAL "Release")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -O3")
else()
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -g -Wall -Wextra")
endif()

# Find SDL2 - metodo alternativo per macOS
if(APPLE)
    # Homebrew installa in /opt/homebrew su Apple Silicon, /usr/local su Intel
    if(EXISTS "/opt/homebrew/include/SDL2")
        set(SDL2_INCLUDE_DIRS "/opt/homebrew/include/SDL2")
        set(SDL2_LIBRARIES "-L/opt/homebrew/lib -lSDL2")
    elseif(EXISTS "/usr/local/include/SDL2")
        set(SDL2_INCLUDE_DIRS "/usr/local/include/SDL2")
        set(SDL2_LIBRARIES "-L/usr/local/lib -lSDL2")
    else()
        # Prova con pkg-config
        find_package(PkgConfig)
        if(PkgConfig_FOUND)
            pkg_check_modules(SDL2 REQUIRED sdl2)
        else()
            message(FATAL_ERROR "SDL2 not found!  Install with: brew install sdl2")
        endif()
    endif()
else()
    # Linux/Windows: usa pkg-config
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(SDL2 REQUIRED sdl2)
endif()

# Include directories
include_directories(
        ${CMAKE_SOURCE_DIR}/include
        ${CMAKE_SOURCE_DIR}/src
        ${SDL2_INCLUDE_DIRS}
)

# Source files
set(SOURCES
        src/main.c
        src/cpu/arm9.c
        src/cpu/arm7.c
        src/cpu/instructions.c
        src/memory/mmu.c
        src/gpu/renderer.c
        src/audio/apu.c
        src/input/input.c
        src/cartridge/rom.c
)

# Header files
set(HEADERS
        include/nds.h
        src/cpu/arm9.h
        src/cpu/arm7.h
        src/cpu/instructions.h
        src/memory/mmu.h
        src/gpu/renderer.h
        src/audio/apu.h
        src/input/input.h
        src/cartridge/rom.h
)

# Executable - NOME CORRETTO
add_executable(ndsEmulator ${SOURCES} ${HEADERS})

# Link SDL2
target_link_libraries(ndsEmulator ${SDL2_LIBRARIES})

# macOS specific frameworks
if(APPLE)
    target_link_libraries(ndsEmulator "-framework Cocoa" "-framework IOKit" "-framework CoreVideo")
endif()