#ifndef MMU_H
#define MMU_H

#include "audio/apu.h"

struct Memory {
    // Main memory regions
    uint8_t* main_ram;      // 4MB Main RAM
    uint8_t* shared_wram;   // 32KB Shared WRAM
    uint8_t* arm9_bios;     // 4KB ARM9 BIOS
    uint8_t* arm7_bios;     // 16KB ARM7 BIOS
    uint8_t* vram;          // 656KB VRAM
    uint8_t* oam;           // 2KB OAM
    uint8_t* palette;       // 2KB Palette RAM
    uint8_t* io_regs;       // I/O Registers

    // ROM data (pointer to cartridge ROM)
    uint8_t* rom;
    uint32_t rom_size;

    // I/O state
    uint16_t keyinput;      // Button state
    uint16_t touchx;        // Touch X coordinate
    uint16_t touchy;        // Touch Y coordinate
};

// Memory management
Memory* memory_create(void);
void    memory_destroy(Memory* memory);
void    memory_reset(Memory* memory);

// Memory access (8/16/32 bit)
uint8_t  memory_read8(Memory* memory, uint32_t address);
uint16_t memory_read16(Memory* memory, uint32_t address);
uint32_t memory_read32(Memory* memory, uint32_t address);

void memory_write8(Memory* memory, uint32_t address, uint8_t value);
void memory_write16(Memory* memory, uint32_t address, uint16_t value);
void memory_write32(Memory* memory, uint32_t address, uint32_t value);

// DMA (simplified)
void memory_dma_transfer(Memory* memory, uint32_t src, uint32_t dst, uint32_t count, uint8_t unit_size);

#endif // MMU_H