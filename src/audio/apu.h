#ifndef APU_H
#define APU_H

#include <stdint.h>
#include <stdbool.h>
#include <SDL.h>

// Forward declaration
typedef struct Memory Memory;

#define APU_SAMPLE_RATE      32768
#define APU_CHANNELS         16
#define APU_BUFFER_SIZE      2048

typedef enum {
    APU_FORMAT_PCM8     = 0,
    APU_FORMAT_PCM16    = 1,
    APU_FORMAT_ADPCM    = 2,
    APU_FORMAT_PSG      = 3
} APUFormat;

typedef struct {
    bool        enabled;
    bool        playing;
    uint32_t    source_addr;
    uint32_t    timer_reload;
    uint32_t    loop_start;
    uint32_t    length;
    uint32_t    current_addr;
    uint32_t    samples_remaining;
    uint8_t     volume;
    uint8_t     volume_div;
    int8_t      pan;
    APUFormat   format;
    int16_t     adpcm_value;
    int8_t      adpcm_index;
    uint32_t    timer_counter;
    int16_t     current_sample;
} APUChannel;

typedef struct APU {
    APUChannel  channels[APU_CHANNELS];
    bool        master_enable;
    uint8_t     master_volume;
    uint16_t     output_bias;
    int16_t     buffer_left[APU_BUFFER_SIZE];
    int16_t     buffer_right[APU_BUFFER_SIZE];
    uint32_t    buffer_pos;
    SDL_AudioDeviceID   audio_device;
    SDL_AudioSpec       audio_spec;
    bool                audio_initialized;
    Memory*     memory;
    uint32_t    cycle_counter;
} APU;

APU*    apu_create(Memory* memory);
void    apu_destroy(APU* apu);
void    apu_reset(APU* apu);
void    apu_step(APU* apu, uint32_t cycles);
void    apu_mix_samples(APU* apu);
void    apu_channel_start(APU* apu, uint8_t channel);
void    apu_channel_stop(APU* apu, uint8_t channel);
void    apu_write_reg(APU* apu, uint32_t addr, uint32_t value);
uint32_t apu_read_reg(APU* apu, uint32_t addr);
void    apu_set_master_enable(APU* apu, bool enable);
void    apu_set_master_volume(APU* apu, uint8_t volume);

#endif // APU_H