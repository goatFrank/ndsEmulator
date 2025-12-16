#ifndef ARM7_H
#define ARM7_H

#include <stdint.h>
#include <stdbool.h>

// Forward declaration
typedef struct Memory Memory;

// Flag bits nel CPSR
#define FLAG_N  (1 << 31)
#define FLAG_Z  (1 << 30)
#define FLAG_C  (1 << 29)
#define FLAG_V  (1 << 28)
#define FLAG_I  (1 << 7)
#define FLAG_F  (1 << 6)
#define FLAG_T  (1 << 5)

// CPU Modes
#define CPU_MODE_USER       0x10
#define CPU_MODE_FIQ        0x11
#define CPU_MODE_IRQ        0x12
#define CPU_MODE_SUPERVISOR 0x13
#define CPU_MODE_ABORT      0x17
#define CPU_MODE_UNDEFINED  0x1B
#define CPU_MODE_SYSTEM     0x1F

// Indirizzi memoria
#define ADDR_ARM7_BIOS      0x00000000

typedef struct ARM7 {
    // General purpose registers R0-R15
    uint32_t r[16];

    // Current Program Status Register
    uint32_t cpsr;

    // Saved Program Status Registers
    uint32_t spsr_fiq;
    uint32_t spsr_irq;
    uint32_t spsr_svc;
    uint32_t spsr_abt;
    uint32_t spsr_und;

    // Banked registers
    uint32_t r8_fiq, r9_fiq, r10_fiq, r11_fiq, r12_fiq;
    uint32_t r13_fiq, r14_fiq;
    uint32_t r13_irq, r14_irq;
    uint32_t r13_svc, r14_svc;
    uint32_t r13_abt, r14_abt;
    uint32_t r13_und, r14_und;

    // Reference to memory system
    Memory* memory;

    // State
    bool halted;
    uint64_t cycles;
} ARM7;

ARM7* arm7_create(Memory* memory);
void  arm7_destroy(ARM7* arm7);
void  arm7_reset(ARM7* arm7);
void  arm7_step(ARM7* arm7);
void  arm7_run_cycles(ARM7* arm7, uint32_t cycles);

static inline bool arm7_get_flag_n(ARM7* cpu) { return (cpu->cpsr & FLAG_N) != 0; }
static inline bool arm7_get_flag_z(ARM7* cpu) { return (cpu->cpsr & FLAG_Z) != 0; }
static inline bool arm7_get_flag_c(ARM7* cpu) { return (cpu->cpsr & FLAG_C) != 0; }
static inline bool arm7_get_flag_v(ARM7* cpu) { return (cpu->cpsr & FLAG_V) != 0; }

static inline bool arm7_in_thumb_mode(ARM7* cpu) { return (cpu->cpsr & FLAG_T) != 0; }

#endif // ARM7_H