#include "rom.h"
#include "../memory/mmu.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// ============================================================================
// Creazione e Distruzione
// ============================================================================

Cartridge* cartridge_create(void) {
    Cartridge* cart = (Cartridge*)calloc(1, sizeof(Cartridge));
    if (!cart) return NULL;
    
    cart->loaded = false;
    cart->data = NULL;
    cart->backup = NULL;
    
    return cart;
}

void cartridge_destroy(Cartridge* cart) {
    if (cart) {
        cartridge_unload(cart);
        free(cart);
    }
}

void cartridge_unload(Cartridge* cart) {
    if (cart->data) {
        free(cart->data);
        cart->data = NULL;
    }
    if (cart->backup) {
        free(cart->backup);
        cart->backup = NULL;
    }
    cart->loaded = false;
    cart->size = 0;
}

// ============================================================================
// Caricamento ROM
// ============================================================================

bool cartridge_load(Cartridge* cart, const char* filepath) {
    if (!cart || !filepath) return false;
    
    // Scarica ROM precedente
    cartridge_unload(cart);
    
    // Apri file
    FILE* file = fopen(filepath, "rb");
    if (!file) {
        printf("Error: Could not open ROM file: %s\n", filepath);
        return false;
    }
    
    // Ottieni dimensione
    fseek(file, 0, SEEK_END);
    cart->size = ftell(file);
    fseek(file, 0, SEEK_SET);
    
    // Verifica dimensione minima (almeno header)
    if (cart->size < 0x200) {
        printf("Error:  ROM file too small (< 512 bytes)\n");
        fclose(file);
        return false;
    }
    
    // Alloca memoria
    cart->data = (uint8_t*)malloc(cart->size);
    if (!cart->data) {
        printf("Error: Could not allocate memory for ROM (%u bytes)\n", cart->size);
        fclose(file);
        return false;
    }
    
    // Leggi file
    size_t read = fread(cart->data, 1, cart->size, file);
    fclose(file);
    
    if (read != cart->size) {
        printf("Error: Could not read entire ROM file\n");
        free(cart->data);
        cart->data = NULL;
        return false;
    }
    
    // Salva path
    strncpy(cart->filepath, filepath, sizeof(cart->filepath) - 1);
    
    // Parsa header
    if (!cartridge_parse_header(cart)) {
        printf("Warning: Could not parse ROM header\n");
    }
    
    cart->loaded = true;
    
    printf("ROM loaded successfully: %s\n", filepath);
    printf("ROM size: %u bytes (%.2f MB)\n", cart->size, cart->size / (1024.0f * 1024.0f));
    
    return true;
}

// ============================================================================
// Parsing Header
// ============================================================================

bool cartridge_parse_header(Cartridge* cart) {
    if (!cart || !cart->data || cart->size < 0x200) {
        return false;
    }
    
    uint8_t* h = cart->data;  // Header pointer
    
    // Game title (offset 0x000, 12 bytes)
    memcpy(cart->header.game_title, &h[0x000], 12);
    cart->header.game_title[12] = '\0';
    
    // Game code (offset 0x00C, 4 bytes)
    memcpy(cart->header.game_code, &h[0x00C], 4);
    cart->header.game_code[4] = '\0';
    
    // Maker code (offset 0x010, 2 bytes)
    memcpy(cart->header.maker_code, &h[0x010], 2);
    cart->header.maker_code[2] = '\0';
    
    // Unit code (offset 0x012)
    cart->header.unit_code = h[0x012];
    
    // Device type (offset 0x013)
    cart->header.device_type = h[0x013];
    
    // Device capacity (offset 0x014)
    cart->header.device_capacity = h[0x014];
    
    // ROM version (offset 0x01E)
    cart->header.rom_version = h[0x01E];
    
    // Autostart (offset 0x01F)
    cart->header.autostart = h[0x01F];
    
    // ARM9 info
    cart->header.arm9_rom_offset = *(uint32_t*)&h[0x020];
    cart->header.arm9_entry_addr = *(uint32_t*)&h[0x024];
    cart->header.arm9_ram_addr   = *(uint32_t*)&h[0x028];
    cart->header.arm9_size       = *(uint32_t*)&h[0x02C];
    
    // ARM7 info
    cart->header.arm7_rom_offset = *(uint32_t*)&h[0x030];
    cart->header.arm7_entry_addr = *(uint32_t*)&h[0x034];
    cart->header.arm7_ram_addr   = *(uint32_t*)&h[0x038];
    cart->header.arm7_size       = *(uint32_t*)&h[0x03C];
    
    // FNT
    cart->header.fnt_offset = *(uint32_t*)&h[0x040];
    cart->header.fnt_size   = *(uint32_t*)&h[0x044];
    
    // FAT
    cart->header.fat_offset = *(uint32_t*)&h[0x048];
    cart->header.fat_size   = *(uint32_t*)&h[0x04C];
    
    // ARM9 Overlay
    cart->header.arm9_overlay_offset = *(uint32_t*)&h[0x050];
    cart->header.arm9_overlay_size   = *(uint32_t*)&h[0x054];
    
    // ARM7 Overlay
    cart->header.arm7_overlay_offset = *(uint32_t*)&h[0x058];
    cart->header.arm7_overlay_size   = *(uint32_t*)&h[0x05C];
    
    // Icon offset
    cart->header.icon_offset = *(uint32_t*)&h[0x068];
    
    // Secure area checksum
    cart->header.secure_checksum = *(uint16_t*)&h[0x06C];
    
    // Secure delay
    cart->header.secure_delay = *(uint16_t*)&h[0x06E];
    
    // Total ROM size (from header at 0x080)
    cart->header.total_size = *(uint32_t*)&h[0x080];
    
    // Header size
    cart->header.header_size = *(uint32_t*)&h[0x084];
    
    return true;
}

void cartridge_print_info(Cartridge* cart) {
    if (!cart || !cart->loaded) {
        printf("No ROM loaded\n");
        return;
    }
    
    printf("\n========== ROM Information ==========\n");
    printf("Title:           %s\n", cart->header.game_title);
    printf("Game Code:      %s\n", cart->header.game_code);
    printf("Maker Code:     %s\n", cart->header.maker_code);
    printf("ROM Version:    %d\n", cart->header.rom_version);
    printf("Unit Code:      0x%02X\n", cart->header.unit_code);
    printf("\n--- ARM9 ---\n");
    printf("ROM Offset:     0x%08X\n", cart->header. arm9_rom_offset);
    printf("Entry Address:   0x%08X\n", cart->header.arm9_entry_addr);
    printf("RAM Address:    0x%08X\n", cart->header. arm9_ram_addr);
    printf("Size:           0x%08X (%u KB)\n", cart->header.arm9_size, cart->header. arm9_size / 1024);
    printf("\n--- ARM7 ---\n");
    printf("ROM Offset:     0x%08X\n", cart->header.arm7_rom_offset);
    printf("Entry Address:  0x%08X\n", cart->header.arm7_entry_addr);
    printf("RAM Address:    0x%08X\n", cart->header.arm7_ram_addr);
    printf("Size:           0x%08X (%u KB)\n", cart->header.arm7_size, cart->header. arm7_size / 1024);
    printf("=====================================\n\n");
}

// ============================================================================
// Caricamento in Memoria
// ============================================================================

bool cartridge_load_to_memory(Cartridge* cart, Memory* memory) {
    if (!cart || !cart->loaded || !memory) {
        return false;
    }
    
    // Carica ARM9 binary
    if (cart->header.arm9_size > 0 && cart->header.arm9_rom_offset + cart->header.arm9_size <= cart->size) {
        uint32_t dst = cart->header.arm9_ram_addr;
        uint32_t src = cart->header.arm9_rom_offset;
        uint32_t size = cart->header.arm9_size;
        
        printf("Loading ARM9 binary:  0x%08X -> 0x%08X (%u bytes)\n", src, dst, size);
        
        for (uint32_t i = 0; i < size; i++) {
            memory_write8(memory, dst + i, cart->data[src + i]);
        }
    }
    
    // Carica ARM7 binary
    if (cart->header.arm7_size > 0 && cart->header.arm7_rom_offset + cart->header.arm7_size <= cart->size) {
        uint32_t dst = cart->header.arm7_ram_addr;
        uint32_t src = cart->header.arm7_rom_offset;
        uint32_t size = cart->header.arm7_size;
        
        printf("Loading ARM7 binary: 0x%08X -> 0x%08X (%u bytes)\n", src, dst, size);
        
        for (uint32_t i = 0; i < size; i++) {
            memory_write8(memory, dst + i, cart->data[src + i]);
        }
    }
    
    // Imposta puntatore ROM in memoria
    memory->rom = cart->data;
    memory->rom_size = cart->size;
    
    return true;
}

// ============================================================================
// Lettura Dati ROM
// ============================================================================

uint8_t cartridge_read8(Cartridge* cart, uint32_t offset) {
    if (!cart || !cart->data || offset >= cart->size) {
        return 0xFF;
    }
    return cart->data[offset];
}

uint16_t cartridge_read16(Cartridge* cart, uint32_t offset) {
    return cartridge_read8(cart, offset) |
           ((uint16_t)cartridge_read8(cart, offset + 1) << 8);
}

uint32_t cartridge_read32(Cartridge* cart, uint32_t offset) {
    return cartridge_read8(cart, offset) |
           ((uint32_t)cartridge_read8(cart, offset + 1) << 8) |
           ((uint32_t)cartridge_read8(cart, offset + 2) << 16) |
           ((uint32_t)cartridge_read8(cart, offset + 3) << 24);
}

// ============================================================================
// Backup Memory
// ============================================================================

bool cartridge_load_backup(Cartridge* cart, const char* filepath) {
    if (!cart) return false;
    
    FILE* file = fopen(filepath, "rb");
    if (!file) {
        // Non è un errore se il save non esiste ancora
        return false;
    }
    
    if (cart->backup && cart->backup_size > 0) {
        fread(cart->backup, 1, cart->backup_size, file);
    }
    
    fclose(file);
    printf("Backup loaded: %s\n", filepath);
    return true;
}

bool cartridge_save_backup(Cartridge* cart, const char* filepath) {
    if (!cart || !cart->backup || cart->backup_size == 0) {
        return false;
    }
    
    FILE* file = fopen(filepath, "wb");
    if (!file) {
        printf("Error: Could not create backup file: %s\n", filepath);
        return false;
    }
    
    fwrite(cart->backup, 1, cart->backup_size, file);
    fclose(file);
    
    printf("Backup saved: %s\n", filepath);
    return true;
}

uint8_t cartridge_backup_read(Cartridge* cart, uint32_t addr) {
    if (!cart || !cart->backup || addr >= cart->backup_size) {
        return 0xFF;
    }
    return cart->backup[addr];
}

void cartridge_backup_write(Cartridge* cart, uint32_t addr, uint8_t value) {
    if (!cart || !cart->backup || addr >= cart->backup_size) {
        return;
    }
    cart->backup[addr] = value;
}

// ============================================================================
// Comandi Cartridge
// ============================================================================

void cartridge_send_command(Cartridge* cart, uint8_t* cmd) {
    if (!cart) return;
    
    memcpy(cart->cmd_buffer, cmd, 8);
    cart->cmd_pos = 0;
    
    // Decodifica comando
    uint8_t cmd_type = cmd[0];
    
    switch (cmd_type) {
        case 0x00:  // Get Header
            cart->transfer_addr = 0;
            cart->transfer_count = 0x200;
            break;
            
        case 0xB7:  // Read Data
            cart->transfer_addr = (cmd[1] << 24) | (cmd[2] << 16) | (cmd[3] << 8) | cmd[4];
            cart->transfer_count = 0x200;  // Normalmente specificato altrove
            break;
            
        case 0xB8:  // Get Chip ID
            cart->transfer_addr = 0;
            cart->transfer_count = 4;
            break;
            
        default:
            cart->transfer_addr = 0;
            cart->transfer_count = 0;
            break;
    }
}

uint32_t cartridge_read_data(Cartridge* cart) {
    if (!cart || cart->transfer_count == 0) {
        return 0xFFFFFFFF;
    }
    
    uint32_t data;
    
    if (cart->cmd_buffer[0] == 0xB8) {
        // Chip ID
        data = 0x00001FC2;  // Dummy chip ID
    } else {
        // Read ROM data
        data = cartridge_read32(cart, cart->transfer_addr);
        cart->transfer_addr += 4;
    }
    
    if (cart->transfer_count >= 4) {
        cart->transfer_count -= 4;
    } else {
        cart->transfer_count = 0;
    }
    
    return data;
}