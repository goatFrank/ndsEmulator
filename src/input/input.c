#include "input.h"
#include "../memory/mmu.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// ============================================================================
// Creazione e Inizializzazione
// ============================================================================

Input* input_create(Memory* memory) {
    Input* input = (Input*)calloc(1, sizeof(Input));
    if (! input) return NULL;
    
    input->memory = memory;
    input_reset(input);
    input_load_default_mapping(input);
    
    return input;
}

void input_destroy(Input* input) {
    if (input) {
        free(input);
    }
}

void input_reset(Input* input) {
    // Tutti i pulsanti rilasciati (active low, quindi tutti 1)
    input->keyinput = 0x03FF;
    input->extkeyin = 0x007F;
    input->keycnt = 0;
    input->prev_keyinput = 0x03FF;
    
    // Touch screen non premuto
    input->touch.pressed = false;
    input->touch.x = 0;
    input->touch.y = 0;
    input->touch.raw_x = 0;
    input->touch.raw_y = 0;
    
    // Aggiorna extkeyin per indicare touch non premuto e console aperta
    input->extkeyin |= KEY_PEN;   // Pen up
    input->extkeyin &= ~KEY_HINGE;  // Console open
}

// ============================================================================
// Mapping Tastiera Default
// ============================================================================

void input_load_default_mapping(Input* input) {
    // Mapping di default (stile comune per emulatori)
    input->key_map[0]  = SDLK_z;        // A
    input->key_map[1]  = SDLK_x;        // B
    input->key_map[2]  = SDLK_RSHIFT;   // Select
    input->key_map[3]  = SDLK_RETURN;   // Start
    input->key_map[4]  = SDLK_RIGHT;    // Right
    input->key_map[5]  = SDLK_LEFT;     // Left
    input->key_map[6]  = SDLK_UP;       // Up
    input->key_map[7]  = SDLK_DOWN;     // Down
    input->key_map[8]  = SDLK_s;        // R
    input->key_map[9]  = SDLK_a;        // L
    input->key_map[10] = SDLK_d;        // X (extended)
    input->key_map[11] = SDLK_c;        // Y (extended)
    
    printf("Input mapping loaded:\n");
    printf("  A=Z, B=X, Start=Enter, Select=RShift\n");
    printf("  D-Pad=Arrow Keys, L=A, R=S, X=D, Y=C\n");
    printf("  Touch=Mouse on bottom screen\n");
}

void input_set_key_mapping(Input* input, uint16_t nds_key, SDL_Keycode sdl_key) {
    // Trova l'indice del tasto NDS
    for (int i = 0; i < 10; i++) {
        if (nds_key == (1 << i)) {
            input->key_map[i] = sdl_key;
            return;
        }
    }
    // X e Y sono extended
    if (nds_key == KEY_X) input->key_map[10] = sdl_key;
    if (nds_key == KEY_Y) input->key_map[11] = sdl_key;
}

// ============================================================================
// Gestione Pulsanti
// ============================================================================

void input_key_down(Input* input, uint16_t key) {
    // Active low:  clear bit per indicare premuto
    input->keyinput &= ~key;
}

void input_key_up(Input* input, uint16_t key) {
    // Active low: set bit per indicare rilasciato
    input->keyinput |= key;
}

bool input_is_key_pressed(Input* input, uint16_t key) {
    // Active low: bit = 0 significa premuto
    return (input->keyinput & key) == 0;
}

// ============================================================================
// Touch Screen
// ============================================================================

void input_touch_down(Input* input, int x, int y) {
    input->touch.pressed = true;
    
    // Limita coordinate allo schermo
    if (x < 0) x = 0;
    if (x > 255) x = 255;
    if (y < 0) y = 0;
    if (y > 191) y = 191;
    
    input->touch.x = (uint16_t)x;
    input->touch.y = (uint16_t)y;
    
    // Converti a valori ADC (NDS usa 12-bit)
    // Range approssimativo: X = 0x000-0xFFF, Y = 0x000-0xFFF
    input->touch.raw_x = (uint16_t)((x * 0xFFF) / 255);
    input->touch.raw_y = (uint16_t)((y * 0xFFF) / 191);
    
    // Aggiorna EXTKEYIN - pen down (clear bit)
    input->extkeyin &= ~KEY_PEN;
}

void input_touch_up(Input* input) {
    input->touch.pressed = false;
    
    // Aggiorna EXTKEYIN - pen up (set bit)
    input->extkeyin |= KEY_PEN;
}

void input_touch_move(Input* input, int x, int y) {
    if (input->touch.pressed) {
        input_touch_down(input, x, y);
    }
}

bool input_is_touching(Input* input) {
    return input->touch.pressed;
}

// ============================================================================
// Gestione Eventi SDL
// ============================================================================

void input_handle_event(Input* input, SDL_Event* event) {
    switch (event->type) {
        case SDL_KEYDOWN: 
            if (event->key.repeat) break;  // Ignora key repeat
            
            // Controlla mapping per ogni tasto
            for (int i = 0; i < 10; i++) {
                if (event->key.keysym. sym == input->key_map[i]) {
                    input_key_down(input, 1 << i);
                }
            }
            // Extended keys (X, Y)
            if (event->key.keysym.sym == input->key_map[10]) {
                input->extkeyin &= ~KEY_X;
            }
            if (event->key. keysym.sym == input->key_map[11]) {
                input->extkeyin &= ~KEY_Y;
            }
            break;
            
        case SDL_KEYUP: 
            // Controlla mapping per ogni tasto
            for (int i = 0; i < 10; i++) {
                if (event->key.keysym.sym == input->key_map[i]) {
                    input_key_up(input, 1 << i);
                }
            }
            // Extended keys (X, Y)
            if (event->key.keysym.sym == input->key_map[10]) {
                input->extkeyin |= KEY_X;
            }
            if (event->key.keysym.sym == input->key_map[11]) {
                input->extkeyin |= KEY_Y;
            }
            break;
            
        case SDL_MOUSEBUTTONDOWN:
            if (event->button.button == SDL_BUTTON_LEFT) {
                // Controlla se il click è sullo schermo inferiore
                // Assumendo che lo schermo inferiore inizi a y = 192 (dopo il top screen)
                int screen_y = event->button.y;
                int screen_x = event->button. x;
                
                // Scala per la finestra (assumendo scala 2x)
                screen_x /= 2;
                screen_y /= 2;
                
                // Bottom screen inizia a y = 192
                if (screen_y >= 192) {
                    input_touch_down(input, screen_x, screen_y - 192);
                }
            }
            break;
            
        case SDL_MOUSEBUTTONUP:
            if (event->button.button == SDL_BUTTON_LEFT) {
                input_touch_up(input);
            }
            break;
            
        case SDL_MOUSEMOTION:
            if (input->touch.pressed) {
                int screen_x = event->motion.x / 2;
                int screen_y = event->motion.y / 2;
                
                if (screen_y >= 192) {
                    input_touch_move(input, screen_x, screen_y - 192);
                }
            }
            break;
    }
}

void input_update(Input* input) {
    // Aggiorna memoria con i registri input
    if (input->memory) {
        input->memory->keyinput = input->keyinput;
    }
    
    // Check per key interrupt
    if (input->keycnt & (1 << 14)) {  // IRQ Enable
        bool irq_cond = (input->keycnt >> 15) & 1;  // AND/OR mode
        uint16_t key_mask = input->keycnt & 0x03FF;
        uint16_t pressed = ~input->keyinput & 0x03FF;
        
        bool trigger = false;
        if (irq_cond) {
            // AND mode: tutti i tasti specificati devono essere premuti
            trigger = (pressed & key_mask) == key_mask;
        } else {
            // OR mode: almeno uno dei tasti specificati deve essere premuto
            trigger = (pressed & key_mask) != 0;
        }
        
        if (trigger) {
            // TODO: Trigger key interrupt
        }
    }
    
    input->prev_keyinput = input->keyinput;
}

// ============================================================================
// Lettura/Scrittura Registri
// ============================================================================

uint16_t input_read_keyinput(Input* input) {
    return input->keyinput;
}

uint16_t input_read_extkeyin(Input* input) {
    return input->extkeyin;
}

uint16_t input_read_keycnt(Input* input) {
    return input->keycnt;
}

void input_write_keycnt(Input* input, uint16_t value) {
    input->keycnt = value;
}