#ifndef INSTRUCTIONS_H
#define INSTRUCTIONS_H

#include <stdint.h>
#include <stdbool.h>

// Forward declarations
typedef struct ARM9 ARM9;
typedef struct ARM7 ARM7;

// ============================================================================
// ARM Mode Instruction Execution
// ============================================================================

void execute_arm(ARM9* arm9, uint32_t instruction);
void execute_arm_arm7(ARM7* arm7, uint32_t instruction);

// ============================================================================
// THUMB Mode Instruction Execution (16-bit)
// ============================================================================

void execute_thumb(ARM9* arm9, uint16_t instruction);
void execute_thumb_arm7(ARM7* arm7, uint16_t instruction);

// ============================================================================
// ALU Operations
// ============================================================================

// Addizione con riporto e overflow
uint32_t alu_add(uint32_t a, uint32_t b, bool* carry_out, bool* overflow_out);

// Sottrazione con riporto e overflow
uint32_t alu_sub(uint32_t a, uint32_t b, bool* carry_out, bool* overflow_out);

// Addizione con carry in ingresso
uint32_t alu_adc(uint32_t a, uint32_t b, bool carry_in, bool* carry_out, bool* overflow_out);

// Sottrazione con carry in ingresso
uint32_t alu_sbc(uint32_t a, uint32_t b, bool carry_in, bool* carry_out, bool* overflow_out);

// ============================================================================
// Barrel Shifter
// ============================================================================

// Tipi di shift
typedef enum {
    SHIFT_LSL = 0,  // Logical Shift Left
    SHIFT_LSR = 1,  // Logical Shift Right
    SHIFT_ASR = 2,  // Arithmetic Shift Right
    SHIFT_ROR = 3   // Rotate Right
} ShiftType;

// Esegue lo shift con barrel shifter
uint32_t barrel_shift(uint32_t value, ShiftType shift_type, uint8_t amount,
                      bool* carry_out, bool immediate_shift);

// ============================================================================
// Condition Checking
// ============================================================================

typedef enum {
    COND_EQ = 0x0,   // Equal (Z=1)
    COND_NE = 0x1,   // Not Equal (Z=0)
    COND_CS = 0x2,   // Carry Set / Unsigned Higher or Same (C=1)
    COND_CC = 0x3,   // Carry Clear / Unsigned Lower (C=0)
    COND_MI = 0x4,   // Minus / Negative (N=1)
    COND_PL = 0x5,   // Plus / Positive or Zero (N=0)
    COND_VS = 0x6,   // Overflow Set (V=1)
    COND_VC = 0x7,   // Overflow Clear (V=0)
    COND_HI = 0x8,   // Unsigned Higher (C=1 && Z=0)
    COND_LS = 0x9,   // Unsigned Lower or Same (C=0 || Z=1)
    COND_GE = 0xA,   // Signed Greater or Equal (N=V)
    COND_LT = 0xB,   // Signed Less Than (N!=V)
    COND_GT = 0xC,   // Signed Greater Than (Z=0 && N=V)
    COND_LE = 0xD,   // Signed Less or Equal (Z=1 || N!=V)
    COND_AL = 0xE,   // Always
    COND_NV = 0xF    // Never (ARMv5+:  unconditional)
} Condition;

// ============================================================================
// ARM Data Processing Opcodes
// ============================================================================

typedef enum {
    DP_AND = 0x0,   // Rd = Rn AND Op2
    DP_EOR = 0x1,   // Rd = Rn XOR Op2
    DP_SUB = 0x2,   // Rd = Rn - Op2
    DP_RSB = 0x3,   // Rd = Op2 - Rn
    DP_ADD = 0x4,   // Rd = Rn + Op2
    DP_ADC = 0x5,   // Rd = Rn + Op2 + C
    DP_SBC = 0x6,   // Rd = Rn - Op2 - ! C
    DP_RSC = 0x7,   // Rd = Op2 - Rn - !C
    DP_TST = 0x8,   // Rn AND Op2 (flags only)
    DP_TEQ = 0x9,   // Rn XOR Op2 (flags only)
    DP_CMP = 0xA,   // Rn - Op2 (flags only)
    DP_CMN = 0xB,   // Rn + Op2 (flags only)
    DP_ORR = 0xC,   // Rd = Rn OR Op2
    DP_MOV = 0xD,   // Rd = Op2
    DP_BIC = 0xE,   // Rd = Rn AND NOT Op2
    DP_MVN = 0xF    // Rd = NOT Op2
} DataProcessingOpcode;

// ============================================================================
// Instruction Type Detection
// ============================================================================

typedef enum {
    INSTR_DATA_PROCESSING,
    INSTR_MULTIPLY,
    INSTR_MULTIPLY_LONG,
    INSTR_SINGLE_DATA_SWAP,
    INSTR_BRANCH_EXCHANGE,
    INSTR_HALFWORD_TRANSFER,
    INSTR_SINGLE_DATA_TRANSFER,
    INSTR_UNDEFINED,
    INSTR_BLOCK_DATA_TRANSFER,
    INSTR_BRANCH,
    INSTR_COPROCESSOR_DATA_TRANSFER,
    INSTR_COPROCESSOR_DATA_OPERATION,
    INSTR_COPROCESSOR_REGISTER_TRANSFER,
    INSTR_SOFTWARE_INTERRUPT,
    INSTR_UNKNOWN
} ARMInstructionType;

ARMInstructionType decode_arm_instruction_type(uint32_t instruction);

#endif // INSTRUCTIONS_H