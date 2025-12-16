#ifndef NDS_H
#define NDS_H

#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

// ============================================================================
// NDS System Constants
// ============================================================================

#define NDS_SCREEN_WIDTH     256
#define NDS_SCREEN_HEIGHT    192
#define NDS_SCREEN_PIXELS    (NDS_SCREEN_WIDTH * NDS_SCREEN_HEIGHT)

#define ARM9_CLOCK_SPEED     67027964
#define ARM7_CLOCK_SPEED     33513982
#define FRAME_RATE           60
#define CYCLES_PER_FRAME     560190

// Memory sizes
#define MAIN_RAM_SIZE        (4 * 1024 * 1024)
#define SHARED_WRAM_SIZE     (32 * 1024)
#define ARM9_BIOS_SIZE       (4 * 1024)
#define ARM7_BIOS_SIZE       (16 * 1024)
#define VRAM_SIZE            (656 * 1024)
#define OAM_SIZE             (2 * 1024)
#define PALETTE_SIZE         (2 * 1024)
#define IO_REGS_SIZE         (4 * 1024)

// Memory map addresses
#define ADDR_ARM7_BIOS       0x00000000
#define ADDR_MAIN_RAM        0x02000000
#define ADDR_SHARED_WRAM     0x03000000
#define ADDR_IO_REGS         0x04000000
#define ADDR_PALETTE         0x05000000
#define ADDR_VRAM            0x06000000
#define ADDR_OAM             0x07000000
#define ADDR_ROM             0x08000000
#define ADDR_ARM9_BIOS       0xFFFF0000

// ============================================================================
// CPU Modes (valori, non enum per evitare conflitti)
// ============================================================================

#define CPU_MODE_USER       0x10
#define CPU_MODE_FIQ        0x11
#define CPU_MODE_IRQ        0x12
#define CPU_MODE_SUPERVISOR 0x13
#define CPU_MODE_ABORT      0x17
#define CPU_MODE_UNDEFINED  0x1B
#define CPU_MODE_SYSTEM     0x1F

// ============================================================================
// CPSR Flag bits
// ============================================================================

#define FLAG_N  (1 << 31)
#define FLAG_Z  (1 << 30)
#define FLAG_C  (1 << 29)
#define FLAG_V  (1 << 28)
#define FLAG_I  (1 << 7)
#define FLAG_F  (1 << 6)
#define FLAG_T  (1 << 5)

// ============================================================================
// Input Buttons
// ============================================================================

#define BTN_A      (1 << 0)
#define BTN_B      (1 << 1)
#define BTN_SELECT (1 << 2)
#define BTN_START  (1 << 3)
#define BTN_RIGHT  (1 << 4)
#define BTN_LEFT   (1 << 5)
#define BTN_UP     (1 << 6)
#define BTN_DOWN   (1 << 7)
#define BTN_R      (1 << 8)
#define BTN_L      (1 << 9)
#define BTN_X      (1 << 10)
#define BTN_Y      (1 << 11)

#endif // NDS_H