#ifndef MMU_H
#define MMU_H

#include <stdint.h>
#include <stdbool.h>
#include "../../include/nds.h"    // ← Aggiungi qui!

typedef struct Memory {
    uint8_t* main_ram;
    uint8_t* shared_wram;
    uint8_t* arm9_bios;
    uint8_t* arm7_bios;
    uint8_t* vram;
    uint8_t* oam;
    uint8_t* palette;
    uint8_t* io_regs;
    uint8_t* rom;
    uint32_t rom_size;
    uint16_t keyinput;
    uint16_t touchx;
    uint16_t touchy;
} Memory;

Memory* memory_create(void);
void    memory_destroy(Memory* memory);
void    memory_reset(Memory* memory);

uint8_t  memory_read8(Memory* memory, uint32_t address);
uint16_t memory_read16(Memory* memory, uint32_t address);
uint32_t memory_read32(Memory* memory, uint32_t address);

void memory_write8(Memory* memory, uint32_t address, uint8_t value);
void memory_write16(Memory* memory, uint32_t address, uint16_t value);
void memory_write32(Memory* memory, uint32_t address, uint32_t value);

#endif // MMU_H