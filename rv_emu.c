#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "rv_emu.h"
#include "bits.h"

#define DEBUG 0

static void unsupported(char *s, uint32_t n) {
    printf("unsupported %s 0x%x\n", s, n);
    exit(-1);
}

void emu_r_type(rv_state *state, uint32_t iw) {
    uint32_t rd = get_bits(iw, 7, 5);
    uint32_t rs1 = get_bits(iw, 15, 5);
    uint32_t rs2 = get_bits(iw, 20, 5);
    uint32_t funct3 = get_bits(iw, 12, 3);
    uint32_t funct7 = get_bits(iw, 25, 7);

    if (funct3 == 0b000 && funct7 == 0b0000000) {       // add
        state->regs[rd] = state->regs[rs1] + state->regs[rs2];
    } else if (funct3 == 0b000 && funct7 == 0b0000001) {    // mul
        state->regs[rd] = state->regs[rs1] * state->regs[rs2];
    } else if (funct3 == 0b000 && funct7 == 0b0100000) {    // sub
        state->regs[rd] = state->regs[rs1] - state->regs[rs2];
    } else if (funct3 == 0b100 && funct7 == 0b0000001) {
        state->regs[rd] = state->regs[rs1] / state->regs[rs2];
    } else if (funct3 == 0b001 && funct7 == 0b0000000) {    // sll/sllw
        state->regs[rd] = state->regs[rs1] << state->regs[rs2];
    } else if (funct3 == 0b101) {    // shift right
        if (funct7 == 0b0000000) {                       // srl/srlw
            state->regs[rd] = state->regs[rs1] >> state->regs[rs2];
        } else if (funct7 == 0b0100000) {                // sra/sraw
            state->regs[rd] =  ((int32_t) state->regs[rs1]) >> state->regs[rs2];
        }
    } else if (funct3 == 0b111 && funct7 == 0b0000000) {    // and
        state->regs[rd] = state->regs[rs1] & state->regs[rs2];
    } else {
        unsupported("R-type funct3", funct3);
    }

    state->analysis.ir_count += 1;
    state->pc += 4; // Next instruction
}

void emu_i_type(rv_state *state, uint32_t iw) {
    uint32_t opcode = get_bits(iw, 0, 7);
    uint32_t rd = get_bits(iw, 7, 5);
    uint32_t funct3 = get_bits(iw, 12, 3);
    uint32_t rs1 = get_bits(iw, 15, 5);
    uint64_t immu = get_bits(iw, 20, 12);
    int64_t imm = sign_extend(immu, 12);
    uint64_t dest = state->regs[rs1] + imm;

    if (opcode == FMT_I_ARITH) {                            // I-type arithmetic
        if (funct3 == 0b000) {                              // addi
            state->regs[rd] = state->regs[rs1] + imm;
        } else if (funct3 == 0b101 && (imm & 0xFC0) == 0b000000) {  // srli                     // srli
            imm = imm & 0b111111;     // get shamt, the lower 6 bits
            state->regs[rd] = state->regs[rs1] >> imm;
        } else {
            unsupported("I-type funct3", funct3);
        }
        state->analysis.ir_count += 1;
    } else {                                                // I-type load
        if (funct3 == 0b000) {      // lb
            state->regs[rd] = *(uint8_t *) dest;
        } else if (funct3 == 0b010) {   // lw
            state->regs[rd] = *(uint32_t *) dest;
        } else if (funct3 == 0b011) {   // ld
            state->regs[rd] = *(uint64_t *) dest;
        } else {
            unsupported("I-type funct3", funct3);
        }
        state->analysis.ld_count += 1;
    }
    
    state->pc += 4; // Next instruction
}

void emu_b_type(rv_state *state, uint32_t iw) {
    uint32_t funct3 = get_bits(iw, 12, 3);
    uint32_t rs1 = get_bits(iw, 15, 5);
    uint32_t rs2 = get_bits(iw, 20, 5);
    uint64_t imm1 = get_bits(iw, 8, 4);
    uint64_t imm2 = get_bits(iw, 25, 5);
    uint64_t imm3 = get_bit(iw, 7);
    uint64_t imm4 = get_bit(iw, 31);
    uint64_t immu = (imm1 << 1) | (imm2 << 5) | (imm3 << 11) | (imm4 << 12);
    int64_t imm = sign_extend(immu, 13);
    bool b_taken = false;

    if (funct3 == 0b000) {                              // beq
        if ((int64_t) state->regs[rs1] == (int64_t) state->regs[rs2]) {
            state->pc += imm;
            b_taken = true;
        } else {
            state->pc += 4;
        }
    } else if (funct3 == 0b100) {                              // blt
        if ((int64_t) state->regs[rs1] < (int64_t) state->regs[rs2]) {
            state->pc += imm;
            b_taken = true;
        } else {
            state->pc += 4;
        }
    } else if (funct3 == 0b101) {                     // bge
        if ((int64_t) state->regs[rs1] >= (int64_t) state->regs[rs2]) {
            state->pc += imm;
            b_taken = true;
        } else {
            state->pc += 4;
        }
    } else if (funct3 == 0b001) {                       // bne
        if ((int64_t) state->regs[rs1] != (int64_t) state->regs[rs2]) {
            state->pc += imm;
            b_taken = true;
        } else {
            state->pc += 4;
        }
    } else {
        unsupported("B-type funct3", funct3);
    }
    if (b_taken) {
        state->analysis.b_taken += 1;
    } else {
        state->analysis.b_not_taken += 1;
    }
}


void emu_j_type(rv_state *state, uint32_t iw) {
    uint32_t rd = get_bits(iw, 7, 5);
    uint64_t imm1 = get_bits(iw, 21, 10);
    uint64_t imm2 = get_bit(iw, 20);
    uint64_t imm3 = get_bits(iw, 12, 8);
    uint64_t imm4 = get_bit(iw, 31);
    uint64_t immu = (imm1 << 1) | (imm2 << 11) | (imm3 << 12) | (imm4 << 20);
    int64_t imm = sign_extend(immu, 21);

    if (rd != 0b0) {
        state->regs[RV_RA] = state->pc + 4;
    }
    state->pc += imm;
    state->analysis.j_count += 1;
}

void emu_s_type(rv_state *state, uint32_t iw) {
    uint32_t funct3 = get_bits(iw, 12, 3);
    uint32_t rs1 = get_bits(iw, 15, 5);
    uint32_t rs2 = get_bits(iw, 20, 5);
    uint64_t imm1 = get_bits(iw, 7, 5);
    uint64_t imm2 = get_bits(iw, 25, 7);
    uint64_t immu = imm1 | (imm2 << 5);
    int64_t imm = sign_extend(immu, 13);
    uint64_t dest = state->regs[rs1] + imm;

    if (funct3 == 0b000) {          // sb
        *(uint8_t *) dest = state->regs[rs2];
    } else if (funct3 == 0b010) {   // sw
        *(uint32_t *) dest = state->regs[rs2];
    } else if (funct3 == 0b011) {   // sd
        *(uint64_t *) dest = state->regs[rs2];
    } else {
        unsupported("S-type funct3", funct3);
    }
    state->pc += 4; // Next instruction
    state->analysis.st_count += 1;
}

void emu_jalr(rv_state *state, uint32_t iw) {
    uint32_t rs1 = (iw >> 15) & 0b1111;  // Will be ra (aka x1)
    uint64_t val = state->regs[rs1];  // Value of regs[1]

    state->pc = val;  // PC = return address
    state->analysis.j_count += 1;
}

static void rv_one(rv_state *state) {
    uint32_t iw  = *((uint32_t*) state->pc);
    iw = cache_lookup(&state->i_cache, (uint64_t) state->pc);

    uint32_t opcode = get_bits(iw, 0, 7);


#if DEBUG
    printf("iw: %08x\n", iw);
#endif
    state->analysis.i_count += 1;

    switch (opcode) {
        case FMT_R_W:
        case FMT_R:
            emu_r_type(state, iw); break;
        case FMT_I_LOAD:
        case FMT_I_ARITH:
            emu_i_type(state, iw); break;
        case FMT_SB:
            emu_b_type(state, iw); break;
        case FMT_UJ:
            emu_j_type(state, iw); break;
        case FMT_S:
            emu_s_type(state, iw); break;
        case FMT_I_JALR:
            emu_jalr(state, iw); break;
        default:
            unsupported("Unknown opcode: ", opcode);
    }
}

void rv_init(rv_state *state, uint32_t *target, 
             uint64_t a0, uint64_t a1, uint64_t a2, uint64_t a3) {
    state->pc = (uint64_t) target;
    state->regs[RV_A0] = a0;
    state->regs[RV_A1] = a1;
    state->regs[RV_A2] = a2;
    state->regs[RV_A3] = a3;

    state->regs[RV_ZERO] = 0;  // zero is always 0 
    state->regs[RV_RA] = RV_STOP;
    state->regs[RV_SP] = (uint64_t) &state->stack[STACK_SIZE];    // Stack pointer will point to first byte just beyond stack 

    memset(&state->analysis, 0, sizeof(rv_analysis));
    cache_init(&state->i_cache);
}

uint64_t rv_emulate(rv_state *state) {
    while (state->pc != RV_STOP) {
        rv_one(state);
    }
    return state->regs[RV_A0];
}

static void print_pct(char *fmt, int numer, int denom) {
    double pct = 0.0;

    if (denom)
        pct = (double) numer / (double) denom * 100.0;
    printf(fmt, numer, pct);
}

void rv_print(rv_analysis *a) {
    int b_total = a->b_taken + a->b_not_taken;

    printf("=== Analysis\n");
    print_pct("Instructions Executed  = %d\n", a->i_count, a->i_count);
    print_pct("R-type + I-type        = %d (%.2f%%)\n", a->ir_count, a->i_count);
    print_pct("Loads                  = %d (%.2f%%)\n", a->ld_count, a->i_count);
    print_pct("Stores                 = %d (%.2f%%)\n", a->st_count, a->i_count);    
    print_pct("Jumps/JAL/JALR         = %d (%.2f%%)\n", a->j_count, a->i_count);
    print_pct("Conditional branches   = %d (%.2f%%)\n", b_total, a->i_count);
    print_pct("  Branches taken       = %d (%.2f%%)\n", a->b_taken, b_total);
    print_pct("  Branches not taken   = %d (%.2f%%)\n", a->b_not_taken, b_total);
}
