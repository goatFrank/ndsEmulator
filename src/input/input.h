#ifndef INPUT_H
#define INPUT_H

#include <stdint.h>
#include <stdbool.h>
#include <SDL.h>

// Forward declaration
typedef struct Memory Memory;

#define KEY_A       (1 << 0)
#define KEY_B       (1 << 1)
#define KEY_SELECT  (1 << 2)
#define KEY_START   (1 << 3)
#define KEY_RIGHT   (1 << 4)
#define KEY_LEFT    (1 << 5)
#define KEY_UP      (1 << 6)
#define KEY_DOWN    (1 << 7)
#define KEY_R       (1 << 8)
#define KEY_L       (1 << 9)
#define KEY_X       (1 << 0)
#define KEY_Y       (1 << 1)
#define KEY_PEN     (1 << 6)
#define KEY_HINGE   (1 << 7)

typedef struct {
    bool        pressed;
    uint16_t    x;
    uint16_t    y;
    uint16_t    raw_x;
    uint16_t    raw_y;
} TouchScreen;

typedef struct Input {
    uint16_t    keyinput;
    uint16_t    extkeyin;
    uint16_t    keycnt;
    TouchScreen touch;
    SDL_Keycode key_map[12];
    Memory*     memory;
    uint16_t    prev_keyinput;
} Input;

Input*  input_create(Memory* memory);
void    input_destroy(Input* input);
void    input_reset(Input* input);
void    input_handle_event(Input* input, SDL_Event* event);
void    input_update(Input* input);
void    input_key_down(Input* input, uint16_t key);
void    input_key_up(Input* input, uint16_t key);
bool    input_is_key_pressed(Input* input, uint16_t key);
void    input_touch_down(Input* input, int x, int y);
void    input_touch_up(Input* input);
void    input_touch_move(Input* input, int x, int y);
bool    input_is_touching(Input* input);
uint16_t input_read_keyinput(Input* input);
uint16_t input_read_extkeyin(Input* input);
uint16_t input_read_keycnt(Input* input);
void     input_write_keycnt(Input* input, uint16_t value);
void    input_set_key_mapping(Input* input, uint16_t nds_key, SDL_Keycode sdl_key);
void    input_load_default_mapping(Input* input);

#endif // INPUT_H