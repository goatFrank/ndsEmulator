#include "mmu.h"

Memory* memory_create(void) {
    Memory* memory = (Memory*)calloc(1, sizeof(Memory));
    if (!memory) return NULL;
    
    // Allocate memory regions
    memory->main_ram = (uint8_t*)calloc(MAIN_RAM_SIZE, 1);
    memory->shared_wram = (uint8_t*)calloc(SHARED_WRAM_SIZE, 1);
    memory->arm9_bios = (uint8_t*)calloc(ARM9_BIOS_SIZE, 1);
    memory->arm7_bios = (uint8_t*)calloc(ARM7_BIOS_SIZE, 1);
    memory->vram = (uint8_t*)calloc(VRAM_SIZE, 1);
    memory->oam = (uint8_t*)calloc(OAM_SIZE, 1);
    memory->palette = (uint8_t*)calloc(PALETTE_SIZE, 1);
    memory->io_regs = (uint8_t*)calloc(IO_REGS_SIZE, 1);
    
    if (!memory->main_ram || ! memory->shared_wram || ! memory->arm9_bios ||
        !memory->arm7_bios || !memory->vram || !memory->oam ||
        !memory->palette || ! memory->io_regs) {
        memory_destroy(memory);
        return NULL;
    }
    
    memory->keyinput = 0x03FF;  // All buttons released
    
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

// Helper to map address to memory region
static inline uint8_t* get_memory_ptr(Memory* memory, uint32_t address, uint32_t* offset) {
    address &= 0x0FFFFFFF;  // Mask out top bits for mirroring
    
    // Main RAM:  0x02000000 - 0x023FFFFF (mirrored)
    if (address >= 0x02000000 && address < 0x03000000) {
        *offset = (address - 0x02000000) % MAIN_RAM_SIZE;
        return memory->main_ram;
    }
    
    // Shared WRAM: 0x03000000 - 0x03FFFFFF
    if (address >= 0x03000000 && address < 0x04000000) {
        *offset = (address - 0x03000000) % SHARED_WRAM_SIZE;
        return memory->shared_wram;
    }
    
    // I/O Registers: 0x04000000 - 0x04FFFFFF
    if (address >= 0x04000000 && address < 0x05000000) {
        *offset = (address - 0x04000000) % IO_REGS_SIZE;
        return memory->io_regs;
    }
    
    // Palette RAM: 0x05000000 - 0x05FFFFFF
    if (address >= 0x05000000 && address < 0x06000000) {
        *offset = (address - 0x05000000) % PALETTE_SIZE;
        return memory->palette;
    }
    
    // VRAM: 0x06000000 - 0x06FFFFFF
    if (address >= 0x06000000 && address < 0x07000000) {
        *offset = (address - 0x06000000) % VRAM_SIZE;
        return memory->vram;
    }
    
    // OAM: 0x07000000 - 0x07FFFFFF
    if (address >= 0x07000000 && address < 0x08000000) {
        *offset = (address - 0x07000000) % OAM_SIZE;
        return memory->oam;
    }
    
    // ROM: 0x08000000+
    if (address >= 0x08000000 && memory->rom != NULL) {
        *offset = (address - 0x08000000) % memory->rom_size;
        return memory->rom;
    }
    
    // ARM9 BIOS: 0xFFFF0000 - 0xFFFFFFFF
    if ((address & 0xFFFF0000) == 0xFFFF0000) {
        *offset = address & 0xFFFF;
        if (*offset < ARM9_BIOS_SIZE) {
            return memory->arm9_bios;
        }
    }
    
    // ARM7 BIOS: 0x00000000 - 0x00003FFF
    if (address < ARM7_BIOS_SIZE) {
        *offset = address;
        return memory->arm7_bios;
    }
    
    *offset = 0;
    return NULL;
}

uint8_t memory_read8(Memory* memory, uint32_t address) {
    // Special I/O registers
    if (address == 0x04000130) return memory->keyinput & 0xFF;
    if (address == 0x04000131) return (memory->keyinput >> 8) & 0xFF;
    
    uint32_t offset;
    uint8_t* ptr = get_memory_ptr(memory, address, &offset);
    return ptr ? ptr[offset] : 0;
}

uint16_t memory_read16(Memory* memory, uint32_t address) {
    address &= ~1;  // Align to 16-bit
    
    // Key input register
    if (address == 0x04000130) return memory->keyinput;
    
    return memory_read8(memory, address) |
           ((uint16_t)memory_read8(memory, address + 1) << 8);
}

uint32_t memory_read32(Memory* memory, uint32_t address) {
    address &= ~3;  // Align to 32-bit
    
    return memory_read8(memory, address) |
           ((uint32_t)memory_read8(memory, address + 1) << 8) |
           ((uint32_t)memory_read8(memory, address + 2) << 16) |
           ((uint32_t)memory_read8(memory, address + 3) << 24);
}

void memory_write8(Memory* memory, uint32_t address, uint8_t value) {
    uint32_t offset;
    uint8_t* ptr = get_memory_ptr(memory, address, &offset);
    
    // Don't write to ROM
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

void memory_dma_transfer(Memory* memory, uint32_t src, uint32_t dst, uint32_t count, uint8_t unit_size) {
    for (uint32_t i = 0; i < count; i++) {
        if (unit_size == 4) {
            memory_write32(memory, dst, memory_read32(memory, src));
            src += 4;
            dst += 4;
        } else {
            memory_write16(memory, dst, memory_read16(memory, src));
            src += 2;
            dst += 2;
        }
    }
}