#include "arm7.h"
#include "instructions.h"
#include "../memory/mmu.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

ARM7* arm7_create(Memory* memory) {
    ARM7* arm7 = (ARM7*)calloc(1, sizeof(ARM7));
    if (!arm7) return NULL;

    arm7->memory = memory;
    arm7_reset(arm7);

    return arm7;
}

void arm7_destroy(ARM7* arm7) {
    if (arm7) {
        free(arm7);
    }
}

void arm7_reset(ARM7* arm7) {
    memset(arm7->r, 0, sizeof(arm7->r));

    arm7->cpsr = CPU_MODE_SUPERVISOR | FLAG_I | FLAG_F;
    arm7->r[15] = ADDR_ARM7_BIOS;
    arm7->r[13] = 0x0380FD80;

    arm7->halted = false;
    arm7->cycles = 0;
}

static bool check_condition_arm7(ARM7* arm7, uint8_t cond) {
    bool n = arm7_get_flag_n(arm7);
    bool z = arm7_get_flag_z(arm7);
    bool c = arm7_get_flag_c(arm7);
    bool v = arm7_get_flag_v(arm7);

    switch (cond) {
        case 0x0: return z;
        case 0x1: return ! z;
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

void arm7_step(ARM7* arm7) {
    if (arm7->halted) {
        arm7->cycles++;
        return;
    }

    if (arm7_in_thumb_mode(arm7)) {
        uint16_t instr = memory_read16(arm7->memory, arm7->r[15]);
        arm7->r[15] += 2;
        execute_thumb_arm7(arm7, instr);
    } else {
        uint32_t instr = memory_read32(arm7->memory, arm7->r[15]);
        arm7->r[15] += 4;

        uint8_t cond = (instr >> 28) & 0xF;
        if (check_condition_arm7(arm7, cond)) {
            execute_arm_arm7(arm7, instr);
        }
    }

    arm7->cycles++;
}

void arm7_run_cycles(ARM7* arm7, uint32_t cycles) {
    for (uint32_t i = 0; i < cycles; i++) {
        arm7_step(arm7);
    }
}