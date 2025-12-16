#ifndef ARM9_H
#define ARM9_H

#include <stdint.h>
#include <stdbool.h>
#include "../../include/nds.h"

// Forward declaration
typedef struct Memory Memory;

typedef struct ARM9 {
    uint32_t r[16];
    uint32_t cpsr;
    uint32_t spsr_fiq;
    uint32_t spsr_irq;
    uint32_t spsr_svc;
    uint32_t spsr_abt;
    uint32_t spsr_und;
    uint32_t r8_fiq, r9_fiq, r10_fiq, r11_fiq, r12_fiq;
    uint32_t r13_fiq, r14_fiq;
    uint32_t r13_irq, r14_irq;
    uint32_t r13_svc, r14_svc;
    uint32_t r13_abt, r14_abt;
    uint32_t r13_und, r14_und;
    Memory* memory;
    bool halted;
    uint64_t cycles;
} ARM9;

ARM9* arm9_create(Memory* memory);
void  arm9_destroy(ARM9* arm9);
void  arm9_reset(ARM9* arm9);
void  arm9_step(ARM9* arm9);
void  arm9_run_cycles(ARM9* arm9, uint32_t cycles);

static inline bool arm9_get_flag_n(ARM9* cpu) { return (cpu->cpsr & FLAG_N) != 0; }
static inline bool arm9_get_flag_z(ARM9* cpu) { return (cpu->cpsr & FLAG_Z) != 0; }
static inline bool arm9_get_flag_c(ARM9* cpu) { return (cpu->cpsr & FLAG_C) != 0; }
static inline bool arm9_get_flag_v(ARM9* cpu) { return (cpu->cpsr & FLAG_V) != 0; }

static inline void arm9_set_flag_n(ARM9* cpu, bool v) { cpu->cpsr = v ? (cpu->cpsr | FLAG_N) : (cpu->cpsr & ~FLAG_N); }
static inline void arm9_set_flag_z(ARM9* cpu, bool v) { cpu->cpsr = v ? (cpu->cpsr | FLAG_Z) : (cpu->cpsr & ~FLAG_Z); }
static inline void arm9_set_flag_c(ARM9* cpu, bool v) { cpu->cpsr = v ? (cpu->cpsr | FLAG_C) : (cpu->cpsr & ~FLAG_C); }
static inline void arm9_set_flag_v(ARM9* cpu, bool v) { cpu->cpsr = v ? (cpu->cpsr | FLAG_V) : (cpu->cpsr & ~FLAG_V); }

static inline bool arm9_in_thumb_mode(ARM9* cpu) { return (cpu->cpsr & FLAG_T) != 0; }

#endif // ARM9_H