#ifndef ROM_H
#define ROM_H

#include <stdint.h>
#include <stdbool.h>

// Forward declaration
typedef struct Memory Memory;

// ============================================================================
// Struttura Header ROM NDS
// ============================================================================

typedef struct {
    // Titolo del gioco (12 caratteri)
    char        game_title[13];

    // Codice gioco (4 caratteri)
    char        game_code[5];

    // Codice produttore (2 caratteri)
    char        maker_code[3];

    // Unit code
    uint8_t     unit_code;

    // Device type
    uint8_t     device_type;

    // Capacità (128KB << n)
    uint8_t     device_capacity;

    // Versione ROM
    uint8_t     rom_version;

    // Flag autostart
    uint8_t     autostart;

    // ARM9 info
    uint32_t    arm9_rom_offset;
    uint32_t    arm9_entry_addr;
    uint32_t    arm9_ram_addr;
    uint32_t    arm9_size;

    // ARM7 info
    uint32_t    arm7_rom_offset;
    uint32_t    arm7_entry_addr;
    uint32_t    arm7_ram_addr;
    uint32_t    arm7_size;

    // File Name Table
    uint32_t    fnt_offset;
    uint32_t    fnt_size;

    // File Allocation Table
    uint32_t    fat_offset;
    uint32_t    fat_size;

    // ARM9 Overlay
    uint32_t    arm9_overlay_offset;
    uint32_t    arm9_overlay_size;

    // ARM7 Overlay
    uint32_t    arm7_overlay_offset;
    uint32_t    arm7_overlay_size;

    // Icon/Title
    uint32_t    icon_offset;

    // Secure area checksum
    uint16_t    secure_checksum;

    // Secure area delay
    uint16_t    secure_delay;

    // Total ROM size
    uint32_t    total_size;

    // Header size
    uint32_t    header_size;

} ROMHeader;

// ============================================================================
// Struttura Cartridge
// ============================================================================

typedef struct Cartridge {
    // Dati ROM
    uint8_t*    data;
    uint32_t    size;

    // Header parsato
    ROMHeader   header;

    // Path del file
    char        filepath[512];

    // Stato
    bool        loaded;

    // Backup memory (SRAM/EEPROM/Flash)
    uint8_t*    backup;
    uint32_t    backup_size;
    uint8_t     backup_type;

    // Comandi cartridge
    uint8_t     cmd_buffer[8];
    uint32_t    cmd_pos;

    // Transfer state
    uint32_t    transfer_addr;
    uint32_t    transfer_count;

} Cartridge;

// ============================================================================
// Tipi di Backup Memory
// ============================================================================

typedef enum {
    BACKUP_NONE     = 0,
    BACKUP_EEPROM_4K    = 1,    // 512 bytes
    BACKUP_EEPROM_64K   = 2,    // 8 KB
    BACKUP_EEPROM_512K  = 3,    // 64 KB
    BACKUP_FLASH_256K   = 4,    // 256 KB
    BACKUP_FLASH_512K   = 5,    // 512 KB
    BACKUP_FRAM         = 6     // 32 KB
} BackupType;

// ============================================================================
// Funzioni Cartridge
// ============================================================================

// Creazione e distruzione
Cartridge*  cartridge_create(void);
void        cartridge_destroy(Cartridge* cart);

// Caricamento ROM
bool        cartridge_load(Cartridge* cart, const char* filepath);
void        cartridge_unload(Cartridge* cart);

// Parsing header
bool        cartridge_parse_header(Cartridge* cart);
void        cartridge_print_info(Cartridge* cart);

// Caricamento in memoria
bool        cartridge_load_to_memory(Cartridge* cart, Memory* memory);

// Lettura dati ROM
uint8_t     cartridge_read8(Cartridge* cart, uint32_t offset);
uint16_t    cartridge_read16(Cartridge* cart, uint32_t offset);
uint32_t    cartridge_read32(Cartridge* cart, uint32_t offset);

// Backup memory
bool        cartridge_load_backup(Cartridge* cart, const char* filepath);
bool        cartridge_save_backup(Cartridge* cart, const char* filepath);
uint8_t     cartridge_backup_read(Cartridge* cart, uint32_t addr);
void        cartridge_backup_write(Cartridge* cart, uint32_t addr, uint8_t value);

// Comandi cartridge (SPI)
void        cartridge_send_command(Cartridge* cart, uint8_t* cmd);
uint32_t    cartridge_read_data(Cartridge* cart);

#endif // ROM_H