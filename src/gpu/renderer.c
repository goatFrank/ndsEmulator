#include "renderer.h"
#include "../memory/mmu.h"
#include <stdio.h>
#include <stdlib.h>

GPU* gpu_create(Memory* memory) {
    GPU* gpu = (GPU*)calloc(1, sizeof(GPU));
    if (!gpu) return NULL;

    gpu->memory = memory;

    // Create window (2x scale)
    gpu->window = SDL_CreateWindow(
        "NDS Emulator",
        SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED,
        NDS_SCREEN_WIDTH * 2,
        NDS_SCREEN_HEIGHT * 2 * 2,  // Two screens stacked
        SDL_WINDOW_SHOWN
    );

    if (!gpu->window) {
        printf("Window creation failed: %s\n", SDL_GetError());
        free(gpu);
        return NULL;
    }

    gpu->renderer = SDL_CreateRenderer(gpu->window, -1,
        SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);

    if (!gpu->renderer) {
        printf("Renderer creation failed:  %s\n", SDL_GetError());
        SDL_DestroyWindow(gpu->window);
        free(gpu);
        return NULL;
    }

    gpu->texture = SDL_CreateTexture(
        gpu->renderer,
        SDL_PIXELFORMAT_ARGB8888,
        SDL_TEXTUREACCESS_STREAMING,
        NDS_SCREEN_WIDTH,
        NDS_SCREEN_HEIGHT * 2
    );

    if (!gpu->texture) {
        printf("Texture creation failed: %s\n", SDL_GetError());
        SDL_DestroyRenderer(gpu->renderer);
        SDL_DestroyWindow(gpu->window);
        free(gpu);
        return NULL;
    }

    gpu_reset(gpu);
    return gpu;
}

void gpu_destroy(GPU* gpu) {
    if (gpu) {
        if (gpu->texture) SDL_DestroyTexture(gpu->texture);
        if (gpu->renderer) SDL_DestroyRenderer(gpu->renderer);
        if (gpu->window) SDL_DestroyWindow(gpu->window);
        free(gpu);
    }
}

void gpu_reset(GPU* gpu) {
    memset(gpu->top_screen, 0, sizeof(gpu->top_screen));
    memset(gpu->bottom_screen, 0, sizeof(gpu->bottom_screen));
    memset(gpu->framebuffer, 0, sizeof(gpu->framebuffer));
    gpu->vcount = 0;
    gpu->dispcnt_a = 0;
    gpu->dispcnt_b = 0;
}

void gpu_render_frame(GPU* gpu) {
    // Semplice rendering di test - colora gli schermi
    // Top screen: blu scuro
    for (int i = 0; i < NDS_SCREEN_PIXELS; i++) {
        gpu->top_screen[i] = 0xFF102040;
    }

    // Bottom screen: verde scuro
    for (int i = 0; i < NDS_SCREEN_PIXELS; i++) {
        gpu->bottom_screen[i] = 0xFF104020;
    }

    // Combina i due schermi nel framebuffer
    memcpy(gpu->framebuffer, gpu->top_screen, sizeof(gpu->top_screen));
    memcpy(gpu->framebuffer + NDS_SCREEN_PIXELS, gpu->bottom_screen, sizeof(gpu->bottom_screen));
}

void gpu_present(GPU* gpu) {
    // Update texture with framebuffer
    SDL_UpdateTexture(gpu->texture, NULL, gpu->framebuffer,
                      NDS_SCREEN_WIDTH * sizeof(uint32_t));

    // Clear and render
    SDL_RenderClear(gpu->renderer);
    SDL_RenderCopy(gpu->renderer, gpu->texture, NULL, NULL);
    SDL_RenderPresent(gpu->renderer);
}