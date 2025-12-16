#include "instructions.h"
#include "arm9.h"
#include "arm7.h"
#include "../memory/mmu.h"
#include <stdio.h>

// ============================================================================
// ALU Operations
// ============================================================================

uint32_t alu_add(uint32_t a, uint32_t b, bool* carry_out, bool* overflow_out) {
    uint64_t result64 = (uint64_t)a + (uint64_t)b;
    uint32_t result = (uint32_t)result64;

    *carry_out = (result64 > 0xFFFFFFFF);

    // Overflow:  segni uguali in input, segno diverso in output
    *overflow_out = ((~(a ^ b)) & (a ^ result) & 0x80000000) != 0;

    return result;
}

uint32_t alu_sub(uint32_t a, uint32_t b, bool* carry_out, bool* overflow_out) {
    uint32_t result = a - b;

    // Carry = NOT borrow (a >= b significa nessun borrow)
    *carry_out = (a >= b);

    // Overflow:  segni diversi in input, risultato ha segno di b
    *overflow_out = ((a ^ b) & (a ^ result) & 0x80000000) != 0;

    return result;
}

uint32_t alu_adc(uint32_t a, uint32_t b, bool carry_in, bool* carry_out, bool* overflow_out) {
    uint64_t result64 = (uint64_t)a + (uint64_t)b + (carry_in ? 1 : 0);
    uint32_t result = (uint32_t)result64;

    *carry_out = (result64 > 0xFFFFFFFF);
    *overflow_out = ((~(a ^ b)) & (a ^ result) & 0x80000000) != 0;

    return result;
}

uint32_t alu_sbc(uint32_t a, uint32_t b, bool carry_in, bool* carry_out, bool* overflow_out) {
    // SBC: a - b - !carry = a + ~b + carry
    return alu_adc(a, ~b, carry_in, carry_out, overflow_out);
}

// ============================================================================
// Barrel Shifter
// ============================================================================

uint32_t barrel_shift(uint32_t value, ShiftType shift_type, uint8_t amount,
                      bool* carry_out, bool immediate_shift) {

    // Caso speciale: shift immediato di 0
    if (amount == 0 && immediate_shift) {
        switch (shift_type) {
            case SHIFT_LSL:
                // LSL #0: nessuna operazione
                return value;

            case SHIFT_LSR:
                // LSR #0 significa LSR #32
                *carry_out = (value >> 31) & 1;
                return 0;

            case SHIFT_ASR:
                // ASR #0 significa ASR #32
                *carry_out = (value >> 31) & 1;
                return (*carry_out) ? 0xFFFFFFFF : 0;

            case SHIFT_ROR:
                // ROR #0 significa RRX (rotate right extended)
                {
                    bool old_carry = *carry_out;
                    *carry_out = value & 1;
                    return (old_carry ? 0x80000000 : 0) | (value >> 1);
                }
        }
    }

    // Shift di 0 (da registro): nessuna operazione
    if (amount == 0) {
        return value;
    }

    switch (shift_type) {
        case SHIFT_LSL:  // Logical Shift Left
            if (amount >= 32) {
                *carry_out = (amount == 32) ? (value & 1) : 0;
                return 0;
            }
            *carry_out = (value >> (32 - amount)) & 1;
            return value << amount;

        case SHIFT_LSR:  // Logical Shift Right
            if (amount >= 32) {
                *carry_out = (amount == 32) ? ((value >> 31) & 1) : 0;
                return 0;
            }
            *carry_out = (value >> (amount - 1)) & 1;
            return value >> amount;

        case SHIFT_ASR:  // Arithmetic Shift Right
            if (amount >= 32) {
                *carry_out = (value >> 31) & 1;
                return (*carry_out) ? 0xFFFFFFFF : 0;
            }
            *carry_out = (value >> (amount - 1)) & 1;
            return (uint32_t)((int32_t)value >> amount);

        case SHIFT_ROR:  // Rotate Right
            amount &= 31;  // Modulo 32
            if (amount == 0) {
                *carry_out = (value >> 31) & 1;
                return value;
            }
            *carry_out = (value >> (amount - 1)) & 1;
            return (value >> amount) | (value << (32 - amount));

        default:
            return value;
    }
}

// ============================================================================
// Instruction Type Decoder
// ============================================================================

ARMInstructionType decode_arm_instruction_type(uint32_t instruction) {
    // Branch Exchange:  0001 0010 1111 1111 1111 0001 xxxx
    if ((instruction & 0x0FFFFFF0) == 0x012FFF10) {
        return INSTR_BRANCH_EXCHANGE;
    }

    // Branch: 101x xxxx xxxx xxxx xxxx xxxx xxxx xxxx
    if ((instruction & 0x0E000000) == 0x0A000000) {
        return INSTR_BRANCH;
    }

    // Software Interrupt: 1111 xxxx xxxx xxxx xxxx xxxx xxxx xxxx
    if ((instruction & 0x0F000000) == 0x0F000000) {
        return INSTR_SOFTWARE_INTERRUPT;
    }

    // Block Data Transfer: 100x xxxx xxxx xxxx xxxx xxxx xxxx xxxx
    if ((instruction & 0x0E000000) == 0x08000000) {
        return INSTR_BLOCK_DATA_TRANSFER;
    }

    // Single Data Transfer: 01xx xxxx xxxx xxxx xxxx xxxx xxxx xxxx
    if ((instruction & 0x0C000000) == 0x04000000) {
        return INSTR_SINGLE_DATA_TRANSFER;
    }

    // Multiply: 0000 00xx xxxx xxxx xxxx 1001 xxxx
    if ((instruction & 0x0FC000F0) == 0x00000090) {
        return INSTR_MULTIPLY;
    }

    // Multiply Long:  0000 1xxx xxxx xxxx xxxx 1001 xxxx
    if ((instruction & 0x0F8000F0) == 0x00800090) {
        return INSTR_MULTIPLY_LONG;
    }

    // Single Data Swap: 0001 0x00 xxxx xxxx xxxx 1001 xxxx
    if ((instruction & 0x0FB00FF0) == 0x01000090) {
        return INSTR_SINGLE_DATA_SWAP;
    }

    // Halfword Transfer: 000x xxxx xxxx xxxx xxxx 1xx1 xxxx
    if ((instruction & 0x0E000090) == 0x00000090) {
        uint8_t bits_7_4 = (instruction >> 4) & 0xF;
        if ((bits_7_4 & 0x9) == 0x9 && bits_7_4 != 0x9) {
            return INSTR_HALFWORD_TRANSFER;
        }
    }

    // Data Processing: 00xx xxxx xxxx xxxx xxxx xxxx xxxx xxxx
    if ((instruction & 0x0C000000) == 0x00000000) {
        return INSTR_DATA_PROCESSING;
    }

    // Coprocessor Data Transfer: 110x xxxx xxxx xxxx xxxx xxxx xxxx xxxx
    if ((instruction & 0x0E000000) == 0x0C000000) {
        return INSTR_COPROCESSOR_DATA_TRANSFER;
    }

    // Coprocessor Data Operation: 1110 xxxx xxxx xxxx xxxx xxx0 xxxx
    if ((instruction & 0x0F000010) == 0x0E000000) {
        return INSTR_COPROCESSOR_DATA_OPERATION;
    }

    // Coprocessor Register Transfer: 1110 xxxx xxxx xxxx xxxx xxx1 xxxx
    if ((instruction & 0x0F000010) == 0x0E000010) {
        return INSTR_COPROCESSOR_REGISTER_TRANSFER;
    }

    return INSTR_UNKNOWN;
}

// ============================================================================
// ARM9 Condition Checking
// ============================================================================

static bool check_condition_arm9(ARM9* cpu, uint8_t cond) {
    bool n = arm9_get_flag_n(cpu);
    bool z = arm9_get_flag_z(cpu);
    bool c = arm9_get_flag_c(cpu);
    bool v = arm9_get_flag_v(cpu);

    switch (cond) {
        case COND_EQ:  return z;
        case COND_NE: return !z;
        case COND_CS: return c;
        case COND_CC: return ! c;
        case COND_MI: return n;
        case COND_PL:  return !n;
        case COND_VS: return v;
        case COND_VC: return !v;
        case COND_HI:  return c && ! z;
        case COND_LS: return !c || z;
        case COND_GE: return n == v;
        case COND_LT: return n != v;
        case COND_GT: return ! z && (n == v);
        case COND_LE: return z || (n != v);
        case COND_AL: return true;
        case COND_NV: return true;  // ARMv5+: unconditional
        default: return false;
    }
}

// ============================================================================
// ARM Data Processing
// ============================================================================

static void arm_data_processing(ARM9* cpu, uint32_t instr) {
    uint8_t opcode = (instr >> 21) & 0xF;
    bool set_flags = (instr >> 20) & 1;
    uint8_t rn = (instr >> 16) & 0xF;
    uint8_t rd = (instr >> 12) & 0xF;

    // Calcola operando 2
    uint32_t operand2;
    bool shift_carry = arm9_get_flag_c(cpu);

    if (instr & (1 << 25)) {
        // Operando immediato con rotazione
        uint32_t imm = instr & 0xFF;
        uint8_t rotate = ((instr >> 8) & 0xF) * 2;

        if (rotate != 0) {
            operand2 = (imm >> rotate) | (imm << (32 - rotate));
            shift_carry = (operand2 >> 31) & 1;
        } else {
            operand2 = imm;
        }
    } else {
        // Operando registro con shift
        uint8_t rm = instr & 0xF;
        operand2 = cpu->r[rm];

        ShiftType shift_type = (ShiftType)((instr >> 5) & 0x3);
        uint8_t shift_amount;

        if (instr & (1 << 4)) {
            // Shift per registro
            uint8_t rs = (instr >> 8) & 0xF;
            shift_amount = cpu->r[rs] & 0xFF;
            operand2 = barrel_shift(operand2, shift_type, shift_amount, &shift_carry, false);
        } else {
            // Shift immediato
            shift_amount = (instr >> 7) & 0x1F;
            operand2 = barrel_shift(operand2, shift_type, shift_amount, &shift_carry, true);
        }
    }

    uint32_t op1 = cpu->r[rn];
    uint32_t result = 0;
    bool carry = false;
    bool overflow = false;
    bool write_result = true;
    bool logical_op = false;

    switch (opcode) {
        case DP_AND:
            result = op1 & operand2;
            logical_op = true;
            break;

        case DP_EOR:
            result = op1 ^ operand2;
            logical_op = true;
            break;

        case DP_SUB:
            result = alu_sub(op1, operand2, &carry, &overflow);
            break;

        case DP_RSB:
            result = alu_sub(operand2, op1, &carry, &overflow);
            break;

        case DP_ADD:
            result = alu_add(op1, operand2, &carry, &overflow);
            break;

        case DP_ADC:
            result = alu_adc(op1, operand2, arm9_get_flag_c(cpu), &carry, &overflow);
            break;

        case DP_SBC:
            result = alu_sbc(op1, operand2, arm9_get_flag_c(cpu), &carry, &overflow);
            break;

        case DP_RSC:
            result = alu_sbc(operand2, op1, arm9_get_flag_c(cpu), &carry, &overflow);
            break;

        case DP_TST:
            result = op1 & operand2;
            write_result = false;
            logical_op = true;
            break;

        case DP_TEQ:
            result = op1 ^ operand2;
            write_result = false;
            logical_op = true;
            break;

        case DP_CMP:
            result = alu_sub(op1, operand2, &carry, &overflow);
            write_result = false;
            break;

        case DP_CMN:
            result = alu_add(op1, operand2, &carry, &overflow);
            write_result = false;
            break;

        case DP_ORR:
            result = op1 | operand2;
            logical_op = true;
            break;

        case DP_MOV:
            result = operand2;
            logical_op = true;
            break;

        case DP_BIC:
            result = op1 & ~operand2;
            logical_op = true;
            break;

        case DP_MVN:
            result = ~operand2;
            logical_op = true;
            break;
    }

    // Scrivi risultato
    if (write_result) {
        cpu->r[rd] = result;
    }

    // Aggiorna flags
    if (set_flags) {
        if (rd == 15 && write_result) {
            // Scrittura a PC con S bit:  ripristina CPSR da SPSR
            // (semplificato - dovrebbe gestire i vari modi)
            cpu->cpsr = cpu->spsr_svc;
        } else {
            arm9_set_flag_n(cpu, (result >> 31) & 1);
            arm9_set_flag_z(cpu, result == 0);

            if (logical_op) {
                arm9_set_flag_c(cpu, shift_carry);
            } else {
                arm9_set_flag_c(cpu, carry);
                arm9_set_flag_v(cpu, overflow);
            }
        }
    }
}

// ============================================================================
// ARM Branch
// ============================================================================

static void arm_branch(ARM9* cpu, uint32_t instr) {
    // Estrai offset 24-bit con segno
    int32_t offset = instr & 0x00FFFFFF;

    // Sign extend da 24 a 32 bit
    if (offset & 0x00800000) {
        offset |= 0xFF000000;
    }

    // Shift left di 2 (allineamento word)
    offset <<= 2;

    // Branch with Link?
    if (instr & (1 << 24)) {
        cpu->r[14] = cpu->r[15] - 4;  // LR = indirizzo istruzione successiva
    }

    // PC già punta a istruzione corrente + 8 (prefetch)
    cpu->r[15] = cpu->r[15] + offset;
}

// ============================================================================
// ARM Branch Exchange
// ============================================================================

static void arm_branch_exchange(ARM9* cpu, uint32_t instr) {
    uint8_t rm = instr & 0xF;
    uint32_t addr = cpu->r[rm];

    // Bit 0 determina il modo:  1 = THUMB, 0 = ARM
    if (addr & 1) {
        cpu->cpsr |= FLAG_T;  // Passa a modo THUMB
        cpu->r[15] = addr & ~1;
    } else {
        cpu->cpsr &= ~FLAG_T;  // Resta in modo ARM
        cpu->r[15] = addr & ~3;
    }
}

// ============================================================================
// ARM Single Data Transfer (LDR/STR)
// ============================================================================

static void arm_single_data_transfer(ARM9* cpu, uint32_t instr) {
    bool immediate = ! ((instr >> 25) & 1);  // Bit 25 = 0 per immediato
    bool pre_index = (instr >> 24) & 1;
    bool add = (instr >> 23) & 1;
    bool byte_transfer = (instr >> 22) & 1;
    bool write_back = (instr >> 21) & 1;
    bool is_load = (instr >> 20) & 1;

    uint8_t rn = (instr >> 16) & 0xF;
    uint8_t rd = (instr >> 12) & 0xF;

    uint32_t base = cpu->r[rn];
    uint32_t offset;

    if (immediate) {
        // Offset immediato 12-bit
        offset = instr & 0xFFF;
    } else {
        // Offset registro con shift
        uint8_t rm = instr & 0xF;
        offset = cpu->r[rm];

        ShiftType shift_type = (ShiftType)((instr >> 5) & 0x3);
        uint8_t shift_amount = (instr >> 7) & 0x1F;
        bool dummy;
        offset = barrel_shift(offset, shift_type, shift_amount, &dummy, true);
    }

    // Calcola indirizzo
    uint32_t addr = add ? (base + offset) : (base - offset);
    uint32_t effective_addr = pre_index ? addr : base;

    if (is_load) {
        // LDR / LDRB
        if (byte_transfer) {
            cpu->r[rd] = memory_read8(cpu->memory, effective_addr);
        } else {
            cpu->r[rd] = memory_read32(cpu->memory, effective_addr);

            // Rotazione per accessi non allineati
            uint8_t misalign = effective_addr & 3;
            if (misalign != 0) {
                cpu->r[rd] = (cpu->r[rd] >> (misalign * 8)) |
                             (cpu->r[rd] << (32 - misalign * 8));
            }
        }
    } else {
        // STR / STRB
        if (byte_transfer) {
            memory_write8(cpu->memory, effective_addr, cpu->r[rd] & 0xFF);
        } else {
            memory_write32(cpu->memory, effective_addr, cpu->r[rd]);
        }
    }

    // Write-back
    if (! pre_index || write_back) {
        if (rn != rd || ! is_load) {  // Non sovrascrivere se Rd == Rn in load
            cpu->r[rn] = addr;
        }
    }
}

// ============================================================================
// ARM Block Data Transfer (LDM/STM)
// ============================================================================

static void arm_block_data_transfer(ARM9* cpu, uint32_t instr) {
    bool pre_index = (instr >> 24) & 1;
    bool add = (instr >> 23) & 1;
    bool psr_force_user = (instr >> 22) & 1;
    bool write_back = (instr >> 21) & 1;
    bool is_load = (instr >> 20) & 1;

    uint8_t rn = (instr >> 16) & 0xF;
    uint16_t reg_list = instr & 0xFFFF;

    uint32_t base = cpu->r[rn];

    // Conta registri nella lista
    int reg_count = 0;
    for (int i = 0; i < 16; i++) {
        if (reg_list & (1 << i)) reg_count++;
    }

    // Calcola indirizzo iniziale
    uint32_t addr;
    if (add) {
        addr = base;
    } else {
        addr = base - (reg_count * 4);
    }

    uint32_t writeback_addr = add ? (base + reg_count * 4) : (base - reg_count * 4);

    // Trasferisci registri
    for (int i = 0; i < 16; i++) {
        if (!(reg_list & (1 << i))) continue;

        uint32_t effective_addr;
        if (add) {
            effective_addr = pre_index ? (addr + 4) : addr;
            addr += 4;
        } else {
            effective_addr = pre_index ? addr : (addr + 4);
            addr += 4;
        }

        if (is_load) {
            cpu->r[i] = memory_read32(cpu->memory, effective_addr);
        } else {
            memory_write32(cpu->memory, effective_addr, cpu->r[i]);
        }
    }

    // Write-back
    if (write_back) {
        cpu->r[rn] = writeback_addr;
    }

    // Gestione flag S per LDM con PC
    if (psr_force_user && is_load && (reg_list & (1 << 15))) {
        cpu->cpsr = cpu->spsr_svc;  // Semplificato
    }
}

// ============================================================================
// ARM Multiply
// ============================================================================

static void arm_multiply(ARM9* cpu, uint32_t instr) {
    bool accumulate = (instr >> 21) & 1;
    bool set_flags = (instr >> 20) & 1;

    uint8_t rd = (instr >> 16) & 0xF;
    uint8_t rn = (instr >> 12) & 0xF;
    uint8_t rs = (instr >> 8) & 0xF;
    uint8_t rm = instr & 0xF;

    uint32_t result = cpu->r[rm] * cpu->r[rs];

    if (accumulate) {
        result += cpu->r[rn];
    }

    cpu->r[rd] = result;

    if (set_flags) {
        arm9_set_flag_n(cpu, (result >> 31) & 1);
        arm9_set_flag_z(cpu, result == 0);
        // C e V sono indefiniti per MUL
    }
}

// ============================================================================
// ARM Multiply Long
// ============================================================================

static void arm_multiply_long(ARM9* cpu, uint32_t instr) {
    bool is_signed = (instr >> 22) & 1;
    bool accumulate = (instr >> 21) & 1;
    bool set_flags = (instr >> 20) & 1;

    uint8_t rd_hi = (instr >> 16) & 0xF;
    uint8_t rd_lo = (instr >> 12) & 0xF;
    uint8_t rs = (instr >> 8) & 0xF;
    uint8_t rm = instr & 0xF;

    uint64_t result;

    if (is_signed) {
        int64_t a = (int32_t)cpu->r[rm];
        int64_t b = (int32_t)cpu->r[rs];
        result = (uint64_t)(a * b);
    } else {
        result = (uint64_t)cpu->r[rm] * (uint64_t)cpu->r[rs];
    }

    if (accumulate) {
        uint64_t acc = ((uint64_t)cpu->r[rd_hi] << 32) | cpu->r[rd_lo];
        result += acc;
    }

    cpu->r[rd_lo] = (uint32_t)result;
    cpu->r[rd_hi] = (uint32_t)(result >> 32);

    if (set_flags) {
        arm9_set_flag_n(cpu, (result >> 63) & 1);
        arm9_set_flag_z(cpu, result == 0);
    }
}

// ============================================================================
// ARM Software Interrupt
// ============================================================================

static void arm_software_interrupt(ARM9* cpu, uint32_t instr) {
    (void)instr;  // Il commento SWI non è usato nell'emulazione

    // Salva stato
    cpu->r14_svc = cpu->r[15] - 4;
    cpu->spsr_svc = cpu->cpsr;

    // Passa a modo Supervisor
    cpu->cpsr = (cpu->cpsr & ~0x1F) | CPU_MODE_SUPERVISOR;
    cpu->cpsr |= FLAG_I;  // Disabilita IRQ

    // Salta al vettore SWI
    cpu->r[15] = 0x00000008;  // ARM9 SWI vector
}

// ============================================================================
// ARM Halfword/Signed Transfer
// ============================================================================

static void arm_halfword_transfer(ARM9* cpu, uint32_t instr) {
    bool pre_index = (instr >> 24) & 1;
    bool add = (instr >> 23) & 1;
    bool immediate = (instr >> 22) & 1;
    bool write_back = (instr >> 21) & 1;
    bool is_load = (instr >> 20) & 1;

    uint8_t rn = (instr >> 16) & 0xF;
    uint8_t rd = (instr >> 12) & 0xF;
    uint8_t op = (instr >> 5) & 0x3;

    uint32_t base = cpu->r[rn];
    uint32_t offset;

    if (immediate) {
        offset = ((instr >> 4) & 0xF0) | (instr & 0xF);
    } else {
        uint8_t rm = instr & 0xF;
        offset = cpu->r[rm];
    }

    uint32_t addr = add ? (base + offset) : (base - offset);
    uint32_t effective_addr = pre_index ?  addr : base;

    if (is_load) {
        switch (op) {
            case 1:  // LDRH - Load unsigned halfword
                cpu->r[rd] = memory_read16(cpu->memory, effective_addr);
                break;
            case 2:  // LDRSB - Load signed byte
                {
                    int8_t val = (int8_t)memory_read8(cpu->memory, effective_addr);
                    cpu->r[rd] = (uint32_t)(int32_t)val;
                }
                break;
            case 3:  // LDRSH - Load signed halfword
                {
                    int16_t val = (int16_t)memory_read16(cpu->memory, effective_addr);
                    cpu->r[rd] = (uint32_t)(int32_t)val;
                }
                break;
        }
    } else {
        // STRH - Store halfword
        if (op == 1) {
            memory_write16(cpu->memory, effective_addr, cpu->r[rd] & 0xFFFF);
        }
    }

    if (! pre_index || write_back) {
        cpu->r[rn] = addr;
    }
}

// ============================================================================
// ARM Single Data Swap
// ============================================================================

static void arm_single_data_swap(ARM9* cpu, uint32_t instr) {
    bool byte_swap = (instr >> 22) & 1;

    uint8_t rn = (instr >> 16) & 0xF;
    uint8_t rd = (instr >> 12) & 0xF;
    uint8_t rm = instr & 0xF;

    uint32_t addr = cpu->r[rn];
    uint32_t temp;

    if (byte_swap) {
        temp = memory_read8(cpu->memory, addr);
        memory_write8(cpu->memory, addr, cpu->r[rm] & 0xFF);
    } else {
        temp = memory_read32(cpu->memory, addr);
        memory_write32(cpu->memory, addr, cpu->r[rm]);
    }

    cpu->r[rd] = temp;
}

// ============================================================================
// Execute ARM Instruction - Main Entry Point
// ============================================================================

void execute_arm(ARM9* cpu, uint32_t instr) {
    // Verifica condizione
    uint8_t cond = (instr >> 28) & 0xF;
    if (! check_condition_arm9(cpu, cond)) {
        return;  // Condizione non soddisfatta, salta
    }

    // Decodifica tipo istruzione
    ARMInstructionType type = decode_arm_instruction_type(instr);

    switch (type) {
        case INSTR_DATA_PROCESSING:
            arm_data_processing(cpu, instr);
            break;

        case INSTR_BRANCH:
            arm_branch(cpu, instr);
            break;

        case INSTR_BRANCH_EXCHANGE:
            arm_branch_exchange(cpu, instr);
            break;

        case INSTR_SINGLE_DATA_TRANSFER:
            arm_single_data_transfer(cpu, instr);
            break;

        case INSTR_BLOCK_DATA_TRANSFER:
            arm_block_data_transfer(cpu, instr);
            break;

        case INSTR_MULTIPLY:
            arm_multiply(cpu, instr);
            break;

        case INSTR_MULTIPLY_LONG:
            arm_multiply_long(cpu, instr);
            break;

        case INSTR_HALFWORD_TRANSFER:
            arm_halfword_transfer(cpu, instr);
            break;

        case INSTR_SINGLE_DATA_SWAP:
            arm_single_data_swap(cpu, instr);
            break;

        case INSTR_SOFTWARE_INTERRUPT:
            arm_software_interrupt(cpu, instr);
            break;

        case INSTR_COPROCESSOR_DATA_TRANSFER:
        case INSTR_COPROCESSOR_DATA_OPERATION:
        case INSTR_COPROCESSOR_REGISTER_TRANSFER:
            // Coprocessor non implementato
            break;

        case INSTR_UNDEFINED:
        case INSTR_UNKNOWN:
        default:
            printf("Unknown ARM instruction: 0x%08X at PC=0x%08X\n",
                   instr, cpu->r[15] - 8);
            break;
    }
}

// ============================================================================
// THUMB Instruction Execution
// ============================================================================

void execute_thumb(ARM9* cpu, uint16_t instr) {
    // Format 1: Move shifted register
    if ((instr & 0xE000) == 0x0000) {
        uint8_t op = (instr >> 11) & 0x3;
        uint8_t offset = (instr >> 6) & 0x1F;
        uint8_t rs = (instr >> 3) & 0x7;
        uint8_t rd = instr & 0x7;

        if (op == 3) {
            // Format 2: Add/Subtract (encoded here)
            goto format_2;
        }

        bool carry = arm9_get_flag_c(cpu);
        uint32_t result = barrel_shift(cpu->r[rs], (ShiftType)op, offset, &carry, true);
        cpu->r[rd] = result;

        arm9_set_flag_n(cpu, (result >> 31) & 1);
        arm9_set_flag_z(cpu, result == 0);
        arm9_set_flag_c(cpu, carry);
        return;
    }

format_2:
    // Format 2: Add/Subtract
    if ((instr & 0xF800) == 0x1800) {
        bool is_imm = (instr >> 10) & 1;
        bool is_sub = (instr >> 9) & 1;
        uint8_t rn_or_imm = (instr >> 6) & 0x7;
        uint8_t rs = (instr >> 3) & 0x7;
        uint8_t rd = instr & 0x7;

        uint32_t operand = is_imm ? rn_or_imm : cpu->r[rn_or_imm];
        bool carry, overflow;
        uint32_t result;

        if (is_sub) {
            result = alu_sub(cpu->r[rs], operand, &carry, &overflow);
        } else {
            result = alu_add(cpu->r[rs], operand, &carry, &overflow);
        }

        cpu->r[rd] = result;
        arm9_set_flag_n(cpu, (result >> 31) & 1);
        arm9_set_flag_z(cpu, result == 0);
        arm9_set_flag_c(cpu, carry);
        arm9_set_flag_v(cpu, overflow);
        return;
    }

    // Format 3: Move/Compare/Add/Subtract immediate
    if ((instr & 0xE000) == 0x2000) {
        uint8_t op = (instr >> 11) & 0x3;
        uint8_t rd = (instr >> 8) & 0x7;
        uint8_t imm = instr & 0xFF;

        bool carry, overflow;
        uint32_t result;

        switch (op) {
            case 0:  // MOV
                cpu->r[rd] = imm;
                arm9_set_flag_n(cpu, 0);
                arm9_set_flag_z(cpu, imm == 0);
                break;

            case 1:  // CMP
                result = alu_sub(cpu->r[rd], imm, &carry, &overflow);
                arm9_set_flag_n(cpu, (result >> 31) & 1);
                arm9_set_flag_z(cpu, result == 0);
                arm9_set_flag_c(cpu, carry);
                arm9_set_flag_v(cpu, overflow);
                break;

            case 2:  // ADD
                result = alu_add(cpu->r[rd], imm, &carry, &overflow);
                cpu->r[rd] = result;
                arm9_set_flag_n(cpu, (result >> 31) & 1);
                arm9_set_flag_z(cpu, result == 0);
                arm9_set_flag_c(cpu, carry);
                arm9_set_flag_v(cpu, overflow);
                break;

            case 3:  // SUB
                result = alu_sub(cpu->r[rd], imm, &carry, &overflow);
                cpu->r[rd] = result;
                arm9_set_flag_n(cpu, (result >> 31) & 1);
                arm9_set_flag_z(cpu, result == 0);
                arm9_set_flag_c(cpu, carry);
                arm9_set_flag_v(cpu, overflow);
                break;
        }
        return;
    }

    // Format 4: ALU operations
    if ((instr & 0xFC00) == 0x4000) {
        uint8_t op = (instr >> 6) & 0xF;
        uint8_t rs = (instr >> 3) & 0x7;
        uint8_t rd = instr & 0x7;

        uint32_t a = cpu->r[rd];
        uint32_t b = cpu->r[rs];
        uint32_t result = 0;
        bool carry = false, overflow = false;
        bool write_result = true;
        bool logical = false;

        switch (op) {
            case 0x0:  // AND
                result = a & b;
                logical = true;
                break;
            case 0x1:  // EOR
                result = a ^ b;
                logical = true;
                break;
            case 0x2:  // LSL
                {
                    bool c = arm9_get_flag_c(cpu);
                    result = barrel_shift(a, SHIFT_LSL, b & 0xFF, &c, false);
                    carry = c;
                    logical = true;
                }
                break;
            case 0x3:  // LSR
                {
                    bool c = arm9_get_flag_c(cpu);
                    result = barrel_shift(a, SHIFT_LSR, b & 0xFF, &c, false);
                    carry = c;
                    logical = true;
                }
                break;
            case 0x4:  // ASR
                {
                    bool c = arm9_get_flag_c(cpu);
                    result = barrel_shift(a, SHIFT_ASR, b & 0xFF, &c, false);
                    carry = c;
                    logical = true;
                }
                break;
            case 0x5:  // ADC
                result = alu_adc(a, b, arm9_get_flag_c(cpu), &carry, &overflow);
                break;
            case 0x6:  // SBC
                result = alu_sbc(a, b, arm9_get_flag_c(cpu), &carry, &overflow);
                break;
            case 0x7:  // ROR
                {
                    bool c = arm9_get_flag_c(cpu);
                    result = barrel_shift(a, SHIFT_ROR, b & 0xFF, &c, false);
                    carry = c;
                    logical = true;
                }
                break;
            case 0x8:  // TST
                result = a & b;
                write_result = false;
                logical = true;
                break;
            case 0x9:  // NEG
                result = alu_sub(0, b, &carry, &overflow);
                break;
            case 0xA:  // CMP
                result = alu_sub(a, b, &carry, &overflow);
                write_result = false;
                break;
            case 0xB:  // CMN
                result = alu_add(a, b, &carry, &overflow);
                write_result = false;
                break;
            case 0xC:  // ORR
                result = a | b;
                logical = true;
                break;
            case 0xD:  // MUL
                result = a * b;
                logical = true;
                break;
            case 0xE:  // BIC
                result = a & ~b;
                logical = true;
                break;
            case 0xF:  // MVN
                result = ~b;
                logical = true;
                break;
        }

        if (write_result) {
            cpu->r[rd] = result;
        }

        arm9_set_flag_n(cpu, (result >> 31) & 1);
        arm9_set_flag_z(cpu, result == 0);
        if (! logical) {
            arm9_set_flag_c(cpu, carry);
            arm9_set_flag_v(cpu, overflow);
        } else if ((op >= 0x2 && op <= 0x4) || op == 0x7) {
            arm9_set_flag_c(cpu, carry);
        }
        return;
    }

    // Format 5: Hi register operations/BX
    if ((instr & 0xFC00) == 0x4400) {
        uint8_t op = (instr >> 8) & 0x3;
        bool h1 = (instr >> 7) & 1;
        bool h2 = (instr >> 6) & 1;
        uint8_t rs = ((instr >> 3) & 0x7) | (h2 ?  8 : 0);
        uint8_t rd = (instr & 0x7) | (h1 ? 8 : 0);

        switch (op) {
            case 0:  // ADD
                cpu->r[rd] += cpu->r[rs];
                break;
            case 1:  // CMP
                {
                    bool c, v;
                    uint32_t result = alu_sub(cpu->r[rd], cpu->r[rs], &c, &v);
                    arm9_set_flag_n(cpu, (result >> 31) & 1);
                    arm9_set_flag_z(cpu, result == 0);
                    arm9_set_flag_c(cpu, c);
                    arm9_set_flag_v(cpu, v);
                }
                break;
            case 2:  // MOV
                cpu->r[rd] = cpu->r[rs];
                break;
            case 3:  // BX
                {
                    uint32_t addr = cpu->r[rs];
                    if (addr & 1) {
                        cpu->cpsr |= FLAG_T;
                        cpu->r[15] = addr & ~1;
                    } else {
                        cpu->cpsr &= ~FLAG_T;
                        cpu->r[15] = addr & ~3;
                    }
                }
                break;
        }
        return;
    }

    // Format 6: PC-relative load
    if ((instr & 0xF800) == 0x4800) {
        uint8_t rd = (instr >> 8) & 0x7;
        uint8_t offset = instr & 0xFF;

        uint32_t addr = (cpu->r[15] & ~3) + (offset << 2);
        cpu->r[rd] = memory_read32(cpu->memory, addr);
        return;
    }

    // Format 7/8: Load/Store with register offset
    if ((instr & 0xF000) == 0x5000) {
        uint8_t op = (instr >> 9) & 0x7;
        uint8_t ro = (instr >> 6) & 0x7;
        uint8_t rb = (instr >> 3) & 0x7;
        uint8_t rd = instr & 0x7;

        uint32_t addr = cpu->r[rb] + cpu->r[ro];

        switch (op) {
            case 0:  // STR
                memory_write32(cpu->memory, addr, cpu->r[rd]);
                break;
            case 1:  // STRH
                memory_write16(cpu->memory, addr, cpu->r[rd] & 0xFFFF);
                break;
            case 2:  // STRB
                memory_write8(cpu->memory, addr, cpu->r[rd] & 0xFF);
                break;
            case 3:  // LDRSB
                cpu->r[rd] = (int32_t)(int8_t)memory_read8(cpu->memory, addr);
                break;
            case 4:  // LDR
                cpu->r[rd] = memory_read32(cpu->memory, addr);
                break;
            case 5:  // LDRH
                cpu->r[rd] = memory_read16(cpu->memory, addr);
                break;
            case 6:  // LDRB
                cpu->r[rd] = memory_read8(cpu->memory, addr);
                break;
            case 7:  // LDRSH
                cpu->r[rd] = (int32_t)(int16_t)memory_read16(cpu->memory, addr);
                break;
        }
        return;
    }

    // Format 9: Load/Store with immediate offset
    if ((instr & 0xE000) == 0x6000) {
        bool is_byte = (instr >> 12) & 1;
        bool is_load = (instr >> 11) & 1;
        uint8_t offset = (instr >> 6) & 0x1F;
        uint8_t rb = (instr >> 3) & 0x7;
        uint8_t rd = instr & 0x7;

        uint32_t addr;
        if (is_byte) {
            addr = cpu->r[rb] + offset;
        } else {
            addr = cpu->r[rb] + (offset << 2);
        }

        if (is_load) {
            if (is_byte) {
                cpu->r[rd] = memory_read8(cpu->memory, addr);
            } else {
                cpu->r[rd] = memory_read32(cpu->memory, addr);
            }
        } else {
            if (is_byte) {
                memory_write8(cpu->memory, addr, cpu->r[rd] & 0xFF);
            } else {
                memory_write32(cpu->memory, addr, cpu->r[rd]);
            }
        }
        return;
    }

    // Format 10: Load/Store halfword
    if ((instr & 0xF000) == 0x8000) {
        bool is_load = (instr >> 11) & 1;
        uint8_t offset = (instr >> 6) & 0x1F;
        uint8_t rb = (instr >> 3) & 0x7;
        uint8_t rd = instr & 0x7;

        uint32_t addr = cpu->r[rb] + (offset << 1);

        if (is_load) {
            cpu->r[rd] = memory_read16(cpu->memory, addr);
        } else {
            memory_write16(cpu->memory, addr, cpu->r[rd] & 0xFFFF);
        }
        return;
    }

    // Format 11: SP-relative load/store
    if ((instr & 0xF000) == 0x9000) {
        bool is_load = (instr >> 11) & 1;
        uint8_t rd = (instr >> 8) & 0x7;
        uint8_t offset = instr & 0xFF;

        uint32_t addr = cpu->r[13] + (offset << 2);

        if (is_load) {
            cpu->r[rd] = memory_read32(cpu->memory, addr);
        } else {
            memory_write32(cpu->memory, addr, cpu->r[rd]);
        }
        return;
    }

    // Format 12: Load address
    if ((instr & 0xF000) == 0xA000) {
        bool is_sp = (instr >> 11) & 1;
        uint8_t rd = (instr >> 8) & 0x7;
        uint8_t offset = instr & 0xFF;

        if (is_sp) {
            cpu->r[rd] = cpu->r[13] + (offset << 2);
        } else {
            cpu->r[rd] = (cpu->r[15] & ~3) + (offset << 2);
        }
        return;
    }

    // Format 13: Add offset to SP
    if ((instr & 0xFF00) == 0xB000) {
        bool is_neg = (instr >> 7) & 1;
        uint8_t offset = (instr & 0x7F) << 2;

        if (is_neg) {
            cpu->r[13] -= offset;
        } else {
            cpu->r[13] += offset;
        }
        return;
    }

    // Format 14: Push/Pop
    if ((instr & 0xF600) == 0xB400) {
        bool is_pop = (instr >> 11) & 1;
        bool pc_lr = (instr >> 8) & 1;
        uint8_t reg_list = instr & 0xFF;

        if (is_pop) {
            // POP
            for (int i = 0; i < 8; i++) {
                if (reg_list & (1 << i)) {
                    cpu->r[i] = memory_read32(cpu->memory, cpu->r[13]);
                    cpu->r[13] += 4;
                }
            }
            if (pc_lr) {
                cpu->r[15] = memory_read32(cpu->memory, cpu->r[13]);
                cpu->r[13] += 4;
            }
        } else {
            // PUSH
            if (pc_lr) {
                cpu->r[13] -= 4;
                memory_write32(cpu->memory, cpu->r[13], cpu->r[14]);
            }
            for (int i = 7; i >= 0; i--) {
                if (reg_list & (1 << i)) {
                    cpu->r[13] -= 4;
                    memory_write32(cpu->memory, cpu->r[13], cpu->r[i]);
                }
            }
        }
        return;
    }

    // Format 15: Multiple load/store
    if ((instr & 0xF000) == 0xC000) {
        bool is_load = (instr >> 11) & 1;
        uint8_t rb = (instr >> 8) & 0x7;
        uint8_t reg_list = instr & 0xFF;

        uint32_t addr = cpu->r[rb];

        for (int i = 0; i < 8; i++) {
            if (reg_list & (1 << i)) {
                if (is_load) {
                    cpu->r[i] = memory_read32(cpu->memory, addr);
                } else {
                    memory_write32(cpu->memory, addr, cpu->r[i]);
                }
                addr += 4;
            }
        }

        // Write back
        if (!(is_load && (reg_list & (1 << rb)))) {
            cpu->r[rb] = addr;
        }
        return;
    }

    // Format 16: Conditional branch
    if ((instr & 0xF000) == 0xD000) {
        uint8_t cond = (instr >> 8) & 0xF;

        if (cond == 0xF) {
            // SWI
            cpu->r14_svc = cpu->r[15] - 2;
            cpu->spsr_svc = cpu->cpsr;
            cpu->cpsr = (cpu->cpsr & ~0x3F) | CPU_MODE_SUPERVISOR | FLAG_I;
            cpu->cpsr &= ~FLAG_T;  // Torna a modo ARM
            cpu->r[15] = 0x00000008;
            return;
        }

        if (! check_condition_arm9(cpu, cond)) {
            return;
        }

        int8_t offset = instr & 0xFF;
        cpu->r[15] += ((int32_t)offset << 1);
        return;
    }

    // Format 18: Unconditional branch
    if ((instr & 0xF800) == 0xE000) {
        int32_t offset = instr & 0x7FF;
        if (offset & 0x400) {
            offset |= 0xFFFFF800;  // Sign extend
        }
        cpu->r[15] += (offset << 1);
        return;
    }

    // Format 19: Long branch with link
    if ((instr & 0xF000) == 0xF000) {
        bool is_second = (instr >> 11) & 1;
        uint32_t offset = instr & 0x7FF;

        if (! is_second) {
            // Prima parte: LR = PC + (offset << 12)
            int32_t signed_offset = offset;
            if (offset & 0x400) {
                signed_offset |= 0xFFFFF800;
            }
            cpu->r[14] = cpu->r[15] + (signed_offset << 12);
        } else {
            // Seconda parte: PC = LR + (offset << 1), LR = old PC | 1
            uint32_t temp = cpu->r[15] - 2;
            cpu->r[15] = cpu->r[14] + (offset << 1);
            cpu->r[14] = temp | 1;
        }
        return;
    }

    printf("Unknown THUMB instruction: 0x%04X at PC=0x%08X\n",
           instr, cpu->r[15] - 4);
}

// ============================================================================
// ARM7 Implementations
// ============================================================================

void execute_arm_arm7(ARM7* cpu, uint32_t instr) {
    // Per ora usa la stessa logica di ARM9 (semplificato)
    // In un'implementazione completa ci sarebbero alcune differenze

    (void)cpu;
    (void)instr;

    // TODO: Implementare le istruzioni ARM7-specifiche
    // Per ora è un placeholder che non fa nulla
}

void execute_thumb_arm7(ARM7* cpu, uint16_t instr) {
    // Per ora usa la stessa logica di ARM9 (semplificato)

    (void)cpu;
    (void)instr;

    // TODO: Implementare le istruzioni THUMB per ARM7
    // Per ora è un placeholder che non fa nulla
    // Implementazione semplificata - stessa logica dell
}