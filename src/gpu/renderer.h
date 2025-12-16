#ifndef RENDERER_H
#define RENDERER_H

#include <stdint.h>
#include <stdbool.h>
#include <SDL.h>

// Forward declaration
typedef struct Memory Memory;

// Costanti schermo NDS
#define NDS_SCREEN_WIDTH     256
#define NDS_SCREEN_HEIGHT    192
#define NDS_SCREEN_PIXELS    (NDS_SCREEN_WIDTH * NDS_SCREEN_HEIGHT)

typedef struct GPU {
    // SDL components
    SDL_Window*   window;
    SDL_Renderer* renderer;
    SDL_Texture*  texture;

    // Framebuffers (RGB888)
    uint32_t top_screen[NDS_SCREEN_PIXELS];
    uint32_t bottom_screen[NDS_SCREEN_PIXELS];

    // Combined framebuffer for texture
    uint32_t framebuffer[NDS_SCREEN_PIXELS * 2];

    // Reference to memory
    Memory* memory;

    // Display control registers
    uint32_t dispcnt_a;
    uint32_t dispcnt_b;

    // Current scanline
    uint16_t vcount;
} GPU;

GPU*  gpu_create(Memory* memory);
void  gpu_destroy(GPU* gpu);
void  gpu_reset(GPU* gpu);
void  gpu_render_frame(GPU* gpu);
void  gpu_present(GPU* gpu);

// Convert NDS color (RGB555) to RGB888
static inline uint32_t color555_to_888(uint16_t color) {
    uint8_t r = ((color >> 0) & 0x1F) << 3;
    uint8_t g = ((color >> 5) & 0x1F) << 3;
    uint8_t b = ((color >> 10) & 0x1F) << 3;
    return (0xFF << 24) | (r << 16) | (g << 8) | b;
}

#endif // RENDERER_H