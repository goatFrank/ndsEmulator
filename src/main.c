#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <SDL.h>

// Include dei moduli dell'emulatore
#include "nds.h"
#include "cpu/arm9.h"
#include "cpu/arm7.h"
#include "memory/mmu.h"
#include "gpu/renderer.h"
#include "audio/apu.h"
#include "input/input.h"
#include "cartridge/rom.h"

// ============================================================================
// Struttura NDS principale
// ============================================================================

typedef struct NDS {
    ARM9*       arm9;
    ARM7*       arm7;
    Memory*     memory;
    GPU*        gpu;
    APU*        apu;
    Input*      input;
    Cartridge*  cartridge;

    bool        running;
    bool        paused;
    uint64_t    total_cycles;
    uint32_t    frame_count;

    // Timing
    uint32_t    target_fps;
    uint32_t    frame_time_ms;
} NDS;

// ============================================================================
// Creazione e Distruzione NDS
// ============================================================================

NDS* nds_create(void) {
    NDS* nds = (NDS*)calloc(1, sizeof(NDS));
    if (!nds) {
        printf("Error: Failed to allocate NDS structure\n");
        return NULL;
    }

    // Crea memoria per prima (altri componenti ne hanno bisogno)
    nds->memory = memory_create();
    if (!nds->memory) {
        printf("Error: Failed to create memory subsystem\n");
        free(nds);
        return NULL;
    }

    // Crea CPU
    nds->arm9 = arm9_create(nds->memory);
    if (!nds->arm9) {
        printf("Error:  Failed to create ARM9 CPU\n");
        memory_destroy(nds->memory);
        free(nds);
        return NULL;
    }

    nds->arm7 = arm7_create(nds->memory);
    if (!nds->arm7) {
        printf("Error: Failed to create ARM7 CPU\n");
        arm9_destroy(nds->arm9);
        memory_destroy(nds->memory);
        free(nds);
        return NULL;
    }

    // Crea GPU
    nds->gpu = gpu_create(nds->memory);
    if (!nds->gpu) {
        printf("Error: Failed to create GPU\n");
        arm7_destroy(nds->arm7);
        arm9_destroy(nds->arm9);
        memory_destroy(nds->memory);
        free(nds);
        return NULL;
    }

    // Crea APU
    nds->apu = apu_create(nds->memory);
    if (!nds->apu) {
        printf("Error: Failed to create APU\n");
        gpu_destroy(nds->gpu);
        arm7_destroy(nds->arm7);
        arm9_destroy(nds->arm9);
        memory_destroy(nds->memory);
        free(nds);
        return NULL;
    }

    // Crea Input
    nds->input = input_create(nds->memory);
    if (!nds->input) {
        printf("Error:  Failed to create input subsystem\n");
        apu_destroy(nds->apu);
        gpu_destroy(nds->gpu);
        arm7_destroy(nds->arm7);
        arm9_destroy(nds->arm9);
        memory_destroy(nds->memory);
        free(nds);
        return NULL;
    }

    // Crea Cartridge
    nds->cartridge = cartridge_create();
    if (!nds->cartridge) {
        printf("Error: Failed to create cartridge subsystem\n");
        input_destroy(nds->input);
        apu_destroy(nds->apu);
        gpu_destroy(nds->gpu);
        arm7_destroy(nds->arm7);
        arm9_destroy(nds->arm9);
        memory_destroy(nds->memory);
        free(nds);
        return NULL;
    }

    // Inizializza stato
    nds->running = false;
    nds->paused = false;
    nds->total_cycles = 0;
    nds->frame_count = 0;
    nds->target_fps = 60;
    nds->frame_time_ms = 1000 / 60;

    printf("NDS Emulator initialized successfully!\n");

    return nds;
}

void nds_destroy(NDS* nds) {
    if (nds) {
        if (nds->cartridge) cartridge_destroy(nds->cartridge);
        if (nds->input) input_destroy(nds->input);
        if (nds->apu) apu_destroy(nds->apu);
        if (nds->gpu) gpu_destroy(nds->gpu);
        if (nds->arm7) arm7_destroy(nds->arm7);
        if (nds->arm9) arm9_destroy(nds->arm9);
        if (nds->memory) memory_destroy(nds->memory);
        free(nds);
        printf("NDS Emulator shut down.\n");
    }
}

// ============================================================================
// Caricamento ROM
// ============================================================================

bool nds_load_rom(NDS* nds, const char* filepath) {
    if (!nds || !filepath) return false;

    // Carica ROM
    if (! cartridge_load(nds->cartridge, filepath)) {
        printf("Error: Failed to load ROM:  %s\n", filepath);
        return false;
    }

    // Stampa info ROM
    cartridge_print_info(nds->cartridge);

    // Carica in memoria
    if (!cartridge_load_to_memory(nds->cartridge, nds->memory)) {
        printf("Error: Failed to load ROM to memory\n");
        return false;
    }

    // Reset CPU con entry points dalla ROM
    arm9_reset(nds->arm9);
    arm7_reset(nds->arm7);

    // Imposta entry points
    nds->arm9->r[15] = nds->cartridge->header.arm9_entry_addr;
    nds->arm7->r[15] = nds->cartridge->header.arm7_entry_addr;

    printf("ARM9 entry point:  0x%08X\n", nds->arm9->r[15]);
    printf("ARM7 entry point: 0x%08X\n", nds->arm7->r[15]);

    return true;
}

// ============================================================================
// Esecuzione Frame
// ============================================================================

void nds_run_frame(NDS* nds) {
    if (!nds || nds->paused) return;

    // Cicli per frame:  ~560,190 per ARM9 a 60fps
    const uint32_t arm9_cycles_per_frame = 560190;
    const uint32_t arm7_cycles_per_frame = 280095;

    // Cicli per scanline
    const uint32_t scanlines = 263;
    const uint32_t arm9_cycles_per_scanline = arm9_cycles_per_frame / scanlines;
    const uint32_t arm7_cycles_per_scanline = arm7_cycles_per_frame / scanlines;

    // Esegui per ogni scanline
    for (uint32_t line = 0; line < scanlines; line++) {
        arm9_run_cycles(nds->arm9, arm9_cycles_per_scanline);
        arm7_run_cycles(nds->arm7, arm7_cycles_per_scanline);
        apu_step(nds->apu, arm9_cycles_per_scanline);
    }

    // Render frame
    gpu_render_frame(nds->gpu);

    // Aggiorna input
    input_update(nds->input);

    nds->total_cycles += arm9_cycles_per_frame;
    nds->frame_count++;
}

// ============================================================================
// Loop Principale
// ============================================================================

static void print_usage(const char* program) {
    printf("Usage: %s <rom. nds>\n", program);
    printf("\nControls:\n");
    printf("  Arrow Keys  - D-Pad\n");
    printf("  Z           - A Button\n");
    printf("  X           - B Button\n");
    printf("  A           - L Button\n");
    printf("  S           - R Button\n");
    printf("  Enter       - Start\n");
    printf("  Right Shift - Select\n");
    printf("  Mouse       - Touch Screen\n");
    printf("\n  P           - Pause/Resume\n");
    printf("  R           - Reset\n");
    printf("  Escape      - Quit\n");
}

int main(int argc, char* argv[]) {
    printf("======================================\n");
    printf("       NDS Emulator v0.1\n");
    printf("======================================\n\n");

    if (argc < 2) {
        print_usage(argv[0]);
        return 1;
    }

    // Inizializza SDL
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_EVENTS) < 0) {
        printf("Error: SDL initialization failed:  %s\n", SDL_GetError());
        return 1;
    }

    // Crea emulatore
    NDS* nds = nds_create();
    if (!nds) {
        SDL_Quit();
        return 1;
    }

    // Carica ROM
    if (!nds_load_rom(nds, argv[1])) {
        nds_destroy(nds);
        SDL_Quit();
        return 1;
    }

    // Loop principale
    nds->running = true;

    Uint32 frame_start;
    Uint32 frame_time;
    Uint32 fps_timer = SDL_GetTicks();
    uint32_t fps_count = 0;
    uint32_t last_frame_count = 0;

    printf("\nStarting emulation...\n\n");

    while (nds->running) {
        frame_start = SDL_GetTicks();

        // Gestione eventi
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            switch (event.type) {
                case SDL_QUIT:
                    nds->running = false;
                    break;

                case SDL_KEYDOWN:
                    if (! event.key.repeat) {
                        if (event.key.keysym.sym == SDLK_ESCAPE) {
                            nds->running = false;
                        } else if (event.key. keysym.sym == SDLK_p) {
                            nds->paused = !nds->paused;
                            printf("%s\n", nds->paused ?  "Paused" : "Resumed");
                        } else if (event.key.keysym.sym == SDLK_r) {
                            arm9_reset(nds->arm9);
                            arm7_reset(nds->arm7);
                            nds->arm9->r[15] = nds->cartridge->header.arm9_entry_addr;
                            nds->arm7->r[15] = nds->cartridge->header.arm7_entry_addr;
                            printf("Reset\n");
                        }
                    }
                    input_handle_event(nds->input, &event);
                    break;

                case SDL_KEYUP:
                case SDL_MOUSEBUTTONDOWN:
                case SDL_MOUSEBUTTONUP:
                case SDL_MOUSEMOTION:
                    input_handle_event(nds->input, &event);
                    break;
            }
        }

        // Esegui frame
        if (! nds->paused) {
            nds_run_frame(nds);
        }

        // Presenta frame
        gpu_present(nds->gpu);

        // FPS counter
        fps_count++;
        if (SDL_GetTicks() - fps_timer >= 1000) {
            char title[64];
            snprintf(title, sizeof(title), "NDS Emulator - %d FPS", fps_count);
            SDL_SetWindowTitle(nds->gpu->window, title);
            fps_count = 0;
            fps_timer = SDL_GetTicks();
        }

        // Frame limiting
        frame_time = SDL_GetTicks() - frame_start;
        if (frame_time < nds->frame_time_ms) {
            SDL_Delay(nds->frame_time_ms - frame_time);
        }
    }

    last_frame_count = nds->frame_count;

    // Cleanup
    nds_destroy(nds);
    SDL_Quit();

    printf("\nEmulation ended.  Total frames: %u\n", last_frame_count);

    return 0;
}