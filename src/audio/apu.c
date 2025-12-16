#include "apu.h"
#include "../memory/mmu.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

// ============================================================================
// Tabelle ADPCM
// ============================================================================

static const int16_t adpcm_index_table[8] = {
    -1, -1, -1, -1, 2, 4, 6, 8
};

static const int16_t adpcm_step_table[89] = {
    7, 8, 9, 10, 11, 12, 13, 14, 16, 17,
    19, 21, 23, 25, 28, 31, 34, 37, 41, 45,
    50, 55, 60, 66, 73, 80, 88, 97, 107, 118,
    130, 143, 157, 173, 190, 209, 230, 253, 279, 307,
    337, 371, 408, 449, 494, 544, 598, 658, 724, 796,
    876, 963, 1060, 1166, 1282, 1411, 1552, 1707, 1878, 2066,
    2272, 2499, 2749, 3024, 3327, 3660, 4026, 4428, 4871, 5358,
    5894, 6484, 7132, 7845, 8630, 9493, 10442, 11487, 12635, 13899,
    15289, 16818, 18500, 20350, 22385, 24623, 27086, 29794, 32767
};

// ============================================================================
// Callback Audio SDL
// ============================================================================

static void audio_callback(void* userdata, Uint8* stream, int len) {
    APU* apu = (APU*)userdata;
    int16_t* out = (int16_t*)stream;
    int samples = len / 4;  // Stereo 16-bit = 4 bytes per sample
    
    for (int i = 0; i < samples; i++) {
        if (apu->buffer_pos > 0) {
            apu->buffer_pos--;
            *out++ = apu->buffer_left[apu->buffer_pos];
            *out++ = apu->buffer_right[apu->buffer_pos];
        } else {
            *out++ = 0;
            *out++ = 0;
        }
    }
}

// ============================================================================
// Creazione e Inizializzazione
// ============================================================================

APU* apu_create(Memory* memory) {
    APU* apu = (APU*)calloc(1, sizeof(APU));
    if (!apu) return NULL;
    
    apu->memory = memory;
    apu_reset(apu);
    
    // Inizializza SDL Audio
    SDL_AudioSpec desired;
    memset(&desired, 0, sizeof(desired));
    desired.freq = 44100;
    desired.format = AUDIO_S16SYS;
    desired.channels = 2;
    desired.samples = 1024;
    desired.callback = audio_callback;
    desired. userdata = apu;
    
    apu->audio_device = SDL_OpenAudioDevice(NULL, 0, &desired, &apu->audio_spec, 0);
    
    if (apu->audio_device > 0) {
        apu->audio_initialized = true;
        SDL_PauseAudioDevice(apu->audio_device, 0);  // Avvia audio
        printf("Audio initialized:  %d Hz, %d channels\n", 
               apu->audio_spec.freq, apu->audio_spec.channels);
    } else {
        printf("Warning: Failed to initialize audio:  %s\n", SDL_GetError());
        apu->audio_initialized = false;
    }
    
    return apu;
}

void apu_destroy(APU* apu) {
    if (apu) {
        if (apu->audio_initialized && apu->audio_device > 0) {
            SDL_CloseAudioDevice(apu->audio_device);
        }
        free(apu);
    }
}

void apu_reset(APU* apu) {
    // Reset tutti i canali
    for (int i = 0; i < APU_CHANNELS; i++) {
        memset(&apu->channels[i], 0, sizeof(APUChannel));
        apu->channels[i].volume = 127;
        apu->channels[i].pan = 0;
    }
    
    apu->master_enable = true;
    apu->master_volume = 127;
    apu->output_bias = 0x200;
    apu->buffer_pos = 0;
    apu->cycle_counter = 0;
}

// ============================================================================
// Decodifica ADPCM
// ============================================================================

static int16_t decode_adpcm_sample(APUChannel* ch, uint8_t nibble) {
    int step = adpcm_step_table[ch->adpcm_index];
    int diff = step >> 3;
    
    if (nibble & 1) diff += step >> 2;
    if (nibble & 2) diff += step >> 1;
    if (nibble & 4) diff += step;
    
    if (nibble & 8) {
        ch->adpcm_value -= diff;
    } else {
        ch->adpcm_value += diff;
    }
    
    // Clamp
    if (ch->adpcm_value > 32767) ch->adpcm_value = 32767;
    if (ch->adpcm_value < -32768) ch->adpcm_value = -32768;
    
    // Aggiorna index
    ch->adpcm_index += adpcm_index_table[nibble & 7];
    if (ch->adpcm_index < 0) ch->adpcm_index = 0;
    if (ch->adpcm_index > 88) ch->adpcm_index = 88;
    
    return ch->adpcm_value;
}

// ============================================================================
// Fetch Sample da Memoria
// ============================================================================

static int16_t fetch_sample(APU* apu, APUChannel* ch) {
    if (! ch->playing || ! ch->enabled) {
        return 0;
    }
    
    int16_t sample = 0;
    
    switch (ch->format) {
        case APU_FORMAT_PCM8:
            {
                int8_t s8 = (int8_t)memory_read8(apu->memory, ch->current_addr);
                sample = s8 << 8;  // Converti a 16-bit
                ch->current_addr++;
            }
            break;
            
        case APU_FORMAT_PCM16:
            sample = (int16_t)memory_read16(apu->memory, ch->current_addr);
            ch->current_addr += 2;
            break;
            
        case APU_FORMAT_ADPCM:
            {
                uint8_t byte = memory_read8(apu->memory, ch->current_addr);
                // Alterna tra nibble basso e alto
                static bool high_nibble = false;
                uint8_t nibble = high_nibble ? (byte >> 4) : (byte & 0xF);
                sample = decode_adpcm_sample(ch, nibble);
                
                if (high_nibble) {
                    ch->current_addr++;
                }
                high_nibble = !high_nibble;
            }
            break;
            
        case APU_FORMAT_PSG: 
            // PSG semplificato (onda quadra)
            sample = (ch->timer_counter & 0x8000) ? 16384 : -16384;
            break;
    }
    
    // Controlla fine del sample
    ch->samples_remaining--;
    if (ch->samples_remaining == 0) {
        if (ch->loop_start > 0) {
            // Loop
            ch->current_addr = ch->source_addr + ch->loop_start;
            ch->samples_remaining = ch->length - ch->loop_start;
        } else {
            // Stop
            ch->playing = false;
        }
    }
    
    return sample;
}

// ============================================================================
// Esecuzione APU
// ============================================================================

void apu_step(APU* apu, uint32_t cycles) {
    if (!apu->master_enable) return;
    
    apu->cycle_counter += cycles;
    
    // Genera sample ogni ~1024 cicli (~32kHz)
    while (apu->cycle_counter >= 1024) {
        apu->cycle_counter -= 1024;
        apu_mix_samples(apu);
    }
}

void apu_mix_samples(APU* apu) {
    int32_t left = 0;
    int32_t right = 0;
    
    // Mixa tutti i canali
    for (int i = 0; i < APU_CHANNELS; i++) {
        APUChannel* ch = &apu->channels[i];
        
        if (! ch->enabled || !ch->playing) continue;
        
        // Timer
        ch->timer_counter += ch->timer_reload;
        if (ch->timer_counter >= 0x10000) {
            ch->timer_counter -= 0x10000;
            ch->current_sample = fetch_sample(apu, ch);
        }
        
        // Applica volume
        int32_t sample = ch->current_sample;
        sample = (sample * ch->volume) >> 7;
        
        // Volume divisor
        sample >>= ch->volume_div;
        
        // Panning
        int pan_left = 64 - ch->pan;
        int pan_right = 64 + ch->pan;
        if (pan_left > 64) pan_left = 64;
        if (pan_right > 64) pan_right = 64;
        if (pan_left < 0) pan_left = 0;
        if (pan_right < 0) pan_right = 0;
        
        left += (sample * pan_left) >> 6;
        right += (sample * pan_right) >> 6;
    }
    
    // Applica master volume
    left = (left * apu->master_volume) >> 7;
    right = (right * apu->master_volume) >> 7;
    
    // Clamp
    if (left > 32767) left = 32767;
    if (left < -32768) left = -32768;
    if (right > 32767) right = 32767;
    if (right < -32768) right = -32768;
    
    // Aggiungi al buffer
    if (apu->buffer_pos < APU_BUFFER_SIZE) {
        apu->buffer_left[apu->buffer_pos] = (int16_t)left;
        apu->buffer_right[apu->buffer_pos] = (int16_t)right;
        apu->buffer_pos++;
    }
}

// ============================================================================
// Controllo Canali
// ============================================================================

void apu_channel_start(APU* apu, uint8_t channel) {
    if (channel >= APU_CHANNELS) return;
    
    APUChannel* ch = &apu->channels[channel];
    ch->enabled = true;
    ch->playing = true;
    ch->current_addr = ch->source_addr;
    ch->samples_remaining = ch->length;
    ch->timer_counter = 0;
    ch->adpcm_value = 0;
    ch->adpcm_index = 0;
}

void apu_channel_stop(APU* apu, uint8_t channel) {
    if (channel >= APU_CHANNELS) return;
    
    apu->channels[channel].enabled = false;
    apu->channels[channel].playing = false;
}

// ============================================================================
// Registri I/O Audio
// ============================================================================

void apu_write_reg(APU* apu, uint32_t addr, uint32_t value) {
    // Sound channel registers:  0x04000400 - 0x040004FF
    if (addr >= 0x04000400 && addr < 0x04000500) {
        uint8_t channel = (addr >> 4) & 0xF;
        uint8_t reg = addr & 0xF;
        
        APUChannel* ch = &apu->channels[channel];
        
        switch (reg) {
            case 0x0:   // CNT (Control)
                ch->volume = value & 0x7F;
                ch->volume_div = (value >> 8) & 0x3;
                ch->pan = ((value >> 16) & 0x7F) - 64;
                ch->format = (APUFormat)((value >> 29) & 0x3);
                
                if (value & (1 << 31)) {
                    apu_channel_start(apu, channel);
                } else {
                    apu_channel_stop(apu, channel);
                }
                break;
                
            case 0x4:  // SAD (Source Address)
                ch->source_addr = value & 0x07FFFFFF;
                break;
                
            case 0x8:  // TMR (Timer)
                ch->timer_reload = value & 0xFFFF;
                ch->loop_start = (value >> 16) & 0xFFFF;
                break;
                
            case 0xC:  // LEN (Length)
                ch->length = value & 0x3FFFFF;
                break;
        }
    }
    
    // Master control:  0x04000500 - 0x04000508
    else if (addr == 0x04000500) {
        apu->master_volume = value & 0x7F;
        apu->master_enable = (value >> 15) & 1;
    }
    else if (addr == 0x04000504) {
        apu->output_bias = value & 0x3FF;
    }
}

uint32_t apu_read_reg(APU* apu, uint32_t addr) {
    if (addr >= 0x04000400 && addr < 0x04000500) {
        uint8_t channel = (addr >> 4) & 0xF;
        uint8_t reg = addr & 0xF;
        
        APUChannel* ch = &apu->channels[channel];
        
        switch (reg) {
            case 0x0:
                return ch->volume | 
                       (ch->volume_div << 8) |
                       ((ch->pan + 64) << 16) |
                       (ch->format << 29) |
                       (ch->playing ? (1 << 31) : 0);
            default:
                return 0;
        }
    }
    
    if (addr == 0x04000500) {
        return apu->master_volume | (apu->master_enable ? (1 << 15) : 0);
    }
    
    return 0;
}

void apu_set_master_enable(APU* apu, bool enable) {
    apu->master_enable = enable;
}

void apu_set_master_volume(APU* apu, uint8_t volume) {
    apu->master_volume = volume & 0x7F;
}