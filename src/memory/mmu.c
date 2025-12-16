#include "mmu.h"
#include "../../include/nds.h"    // ← AGGIUNGI QUESTA RIGA!
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

Memory* memory_create(void) {
    Memory* memory = (Memory*)calloc(1, sizeof(Memory));
    if (!memory) return NULL;

    memory->main_ram = (uint8_t*)calloc(MAIN_RAM_SIZE, 1);
    memory->shared_wram = (uint8_t*)calloc(SHARED_WRAM_SIZE, 1);
    memory->arm9_bios = (uint8_t*)calloc(ARM9_BIOS_SIZE, 1);
    memory->arm7_bios = (uint8_t*)calloc(ARM7_BIOS_SIZE, 1);
    memory->vram = (uint8_t*)calloc(VRAM_SIZE, 1);
    memory->oam = (uint8_t*)calloc(OAM_SIZE, 1);
    memory->palette = (uint8_t*)calloc(PALETTE_SIZE, 1);
    memory->io_regs = (uint8_t*)calloc(IO_REGS_SIZE, 1);

    if (!memory->main_ram || !memory->shared_wram || !memory->arm9_bios ||
        !memory->arm7_bios || !memory->vram || !memory->oam ||
        !memory->palette || ! memory->io_regs) {
        memory_destroy(memory);
        return NULL;
    }

    memory->keyinput = 0x03FF;
    memory->rom = NULL;
    memory->rom_size = 0;

    return memory;
}

void memory_destroy(Memory* memory) {
    if (memory) {
        free(memory->main_ram);
        free(memory->shared_wram);
        free(memory->arm9_bios);
        free(memory->arm7_bios);
        free(memory->vram);
        free(memory->oam);
        free(memory->palette);
        free(memory->io_regs);
        free(memory);
    }
}

void memory_reset(Memory* memory) {
    memset(memory->main_ram, 0, MAIN_RAM_SIZE);
    memset(memory->shared_wram, 0, SHARED_WRAM_SIZE);
    memset(memory->vram, 0, VRAM_SIZE);
    memset(memory->oam, 0, OAM_SIZE);
    memset(memory->palette, 0, PALETTE_SIZE);
    memset(memory->io_regs, 0, IO_REGS_SIZE);
    memory->keyinput = 0x03FF;
}

static inline uint8_t* get_memory_ptr(Memory* memory, uint32_t address, uint32_t* offset) {
    address &= 0x0FFFFFFF;

    if (address >= 0x02000000 && address < 0x03000000) {
        *offset = (address - 0x02000000) % MAIN_RAM_SIZE;
        return memory->main_ram;
    }

    if (address >= 0x03000000 && address < 0x04000000) {
        *offset = (address - 0x03000000) % SHARED_WRAM_SIZE;
        return memory->shared_wram;
    }

    if (address >= 0x04000000 && address < 0x05000000) {
        *offset = (address - 0x04000000) % IO_REGS_SIZE;
        return memory->io_regs;
    }

    if (address >= 0x05000000 && address < 0x06000000) {
        *offset = (address - 0x05000000) % PALETTE_SIZE;
        return memory->palette;
    }

    if (address >= 0x06000000 && address < 0x07000000) {
        *offset = (address - 0x06000000) % VRAM_SIZE;
        return memory->vram;
    }

    if (address >= 0x07000000 && address < 0x08000000) {
        *offset = (address - 0x07000000) % OAM_SIZE;
        return memory->oam;
    }

    if (address >= 0x08000000 && memory->rom != NULL) {
        *offset = (address - 0x08000000) % memory->rom_size;
        return memory->rom;
    }

    if ((address & 0xFFFF0000) == 0xFFFF0000) {
        *offset = address & 0xFFFF;
        if (*offset < ARM9_BIOS_SIZE) {
            return memory->arm9_bios;
        }
    }

    if (address < ARM7_BIOS_SIZE) {
        *offset = address;
        return memory->arm7_bios;
    }

    *offset = 0;
    return NULL;
}

uint8_t memory_read8(Memory* memory, uint32_t address) {
    if (address == 0x04000130) return memory->keyinput & 0xFF;
    if (address == 0x04000131) return (memory->keyinput >> 8) & 0xFF;

    uint32_t offset;
    uint8_t* ptr = get_memory_ptr(memory, address, &offset);
    return ptr ?  ptr[offset] : 0;
}

uint16_t memory_read16(Memory* memory, uint32_t address) {
    address &= ~1;
    if (address == 0x04000130) return memory->keyinput;

    return memory_read8(memory, address) |
           ((uint16_t)memory_read8(memory, address + 1) << 8);
}

uint32_t memory_read32(Memory* memory, uint32_t address) {
    address &= ~3;

    return memory_read8(memory, address) |
           ((uint32_t)memory_read8(memory, address + 1) << 8) |
           ((uint32_t)memory_read8(memory, address + 2) << 16) |
           ((uint32_t)memory_read8(memory, address + 3) << 24);
}

void memory_write8(Memory* memory, uint32_t address, uint8_t value) {
    uint32_t offset;
    uint8_t* ptr = get_memory_ptr(memory, address, &offset);

    if (ptr == memory->rom) return;

    if (ptr) {
        ptr[offset] = value;
    }
}

void memory_write16(Memory* memory, uint32_t address, uint16_t value) {
    address &= ~1;
    memory_write8(memory, address, value & 0xFF);
    memory_write8(memory, address + 1, (value >> 8) & 0xFF);
}

void memory_write32(Memory* memory, uint32_t address, uint32_t value) {
    address &= ~3;
    memory_write8(memory, address, value & 0xFF);
    memory_write8(memory, address + 1, (value >> 8) & 0xFF);
    memory_write8(memory, address + 2, (value >> 16) & 0xFF);
    memory_write8(memory, address + 3, (value >> 24) & 0xFF);
}