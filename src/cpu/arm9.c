#include "arm9.h"
#include "instructions.h"
#include "../memory/mmu.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

ARM9* arm9_create(Memory* memory) {
    ARM9* arm9 = (ARM9*)calloc(1, sizeof(ARM9));
    if (!arm9) return NULL;

    arm9->memory = memory;
    arm9_reset(arm9);

    return arm9;
}

void arm9_destroy(ARM9* arm9) {
    if (arm9) {
        free(arm9);
    }
}

void arm9_reset(ARM9* arm9) {
    memset(arm9->r, 0, sizeof(arm9->r));

    arm9->cpsr = CPU_MODE_SUPERVISOR | FLAG_I | FLAG_F;
    arm9->r[15] = ADDR_ARM9_BIOS;    // ← r[15] invece di PC
    arm9->r[13] = 0x03002F7C;        // ← r[13] invece di SP

    arm9->halted = false;
    arm9->cycles = 0;
}

static bool check_condition(ARM9* arm9, uint8_t cond) {
    bool n = arm9_get_flag_n(arm9);
    bool z = arm9_get_flag_z(arm9);
    bool c = arm9_get_flag_c(arm9);
    bool v = arm9_get_flag_v(arm9);

    switch (cond) {
        case 0x0: return z;
        case 0x1: return !z;
        case 0x2: return c;
        case 0x3: return !c;
        case 0x4: return n;
        case 0x5: return !n;
        case 0x6: return v;
        case 0x7: return !v;
        case 0x8: return c && ! z;
        case 0x9: return !c || z;
        case 0xA: return n == v;
        case 0xB: return n != v;
        case 0xC: return ! z && (n == v);
        case 0xD:  return z || (n != v);
        case 0xE: return true;
        case 0xF: return true;
        default: return false;
    }
}

void arm9_step(ARM9* arm9) {
    if (arm9->halted) {
        arm9->cycles++;
        return;
    }

    if (arm9_in_thumb_mode(arm9)) {
        uint16_t instr = memory_read16(arm9->memory, arm9->r[15]);  // ← r[15]
        arm9->r[15] += 2;                                           // ← r[15]
        execute_thumb(arm9, instr);
    } else {
        uint32_t instr = memory_read32(arm9->memory, arm9->r[15]);  // ← r[15]
        arm9->r[15] += 4;                                           // ← r[15]

        uint8_t cond = (instr >> 28) & 0xF;
        if (check_condition(arm9, cond)) {
            execute_arm(arm9, instr);
        }
    }

    arm9->cycles++;
}

void arm9_run_cycles(ARM9* arm9, uint32_t cycles) {
    for (uint32_t i = 0; i < cycles; i++) {
        arm9_step(arm9);
    }
}