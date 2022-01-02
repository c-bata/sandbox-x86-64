#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "instruction.h"
#include "emulator.h"
#include "emulator_function.h"
#include "modrm.h"
#include "io.h"

instruction_func_t* instructions[256];

static void mov_r8_imm8(Emulator* emu) {
    // uint8_t reg= get_code8(emu, 0) - 0xB0;
    // uint8_t imm8 = get_code8(emu, 1);
    // set_register8(emu, reg, imm8);
    // emu->rip += 2;
}

static void mov_r32_imm32(Emulator* emu) {
    // uint8_t reg = get_code8(emu, 0) - 0xB8;
    // uint32_t value = get_code32(emu, 1);
    // emu->registers[reg] = value;
    // emu->rip += 5;  // opcode 1 byte, operand 4 bytes
}

static void mov_rm32_imm32(Emulator* emu) {
    // emu->rip += 1;
    // ModRM modrm;
    // parse_modrm(emu, &modrm);
    // uint32_t value = get_code32(emu, 0);
    // emu->rip += 4;  // operand 4 bytes
    // set_rm32(emu, &modrm, value);
}

static void mov_r32_rm32(Emulator* emu) {
    // emu->rip += 1;
    // ModRM modrm;
    // parse_modrm(emu, &modrm);
    // uint32_t r32 = get_rm32(emu, &modrm);
    // set_r32(emu, &modrm, r32);
}

static void mov_rm32_r32(Emulator* emu) {
    // emu->rip += 1;
    // ModRM modrm;
    // parse_modrm(emu, &modrm);
    // uint32_t r32 = get_r32(emu, &modrm);
    // set_rm32(emu, &modrm, r32);
}

static void mov_rm8_r8(Emulator* emu) {
    // emu->rip += 1;
    // ModRM modrm;
    // parse_modrm(emu, &modrm);
    // uint8_t r8 = get_r8(emu, &modrm);
    // set_rm8(emu, &modrm, r8);
}

static void mov_r8_rm8(Emulator* emu) {
    // emu->rip += 1;
    // ModRM modrm;
    // parse_modrm(emu, &modrm);
    // uint8_t rm8 = get_rm8(emu, &modrm);
    // set_r8(emu, &modrm, rm8);
}

static void add_rm32_r32(Emulator* emu) {
    // emu->rip += 1;
    // ModRM modrm;
    // parse_modrm(emu, &modrm);
    // uint32_t r32 = get_r32(emu, &modrm);
    // uint32_t rm32 = get_rm32(emu, &modrm);
    // set_rm32(emu, &modrm, rm32 + r32);
}

static void cmp_r32_rm32(Emulator* emu) {
    // emu->rip += 1;
    // ModRM modrm;
    // parse_modrm(emu, &modrm);
    // uint32_t r32 = get_r32(emu, &modrm);
    // uint32_t rm32 = get_rm32(emu, &modrm);
    // uint64_t result = (uint64_t) r32 - (uint64_t) rm32;
    // update_rflags_sub(emu, r32, rm32, result);
}

static void cmp_al_imm8(Emulator* emu) {
    // uint8_t al = get_register8(emu, AL);
    // uint8_t imm8 = get_code8(emu, 1);
    // uint64_t result = (uint64_t) al - (uint64_t) imm8;
    // update_rflags_sub(emu, al, imm8, result);
    // emu->rip += 2;
}

static void add_rm32_imm8(Emulator* emu, ModRM* modrm) {
    // uint32_t rm32 = get_rm32(emu, modrm);
    // uint32_t imm8 = (int32_t)get_sign_code8(emu, 0);
    // emu->rip += 1;
    // set_rm32(emu, modrm, rm32 + imm8);
}

static void sub_rm32_imm8(Emulator* emu, ModRM* modrm) {
    // uint32_t rm32 = get_rm32(emu, modrm);
    // uint32_t imm8 = (int32_t) get_sign_code8(emu, 0); // 型変換が謎
    // emu->rip += 1;
    // uint64_t result = (uint64_t)rm32 - (uint64_t)imm8;
    // set_rm32(emu, modrm, (uint32_t) result);
    // update_rflags_sub(emu, rm32, imm8, result);
}

static void cmp_rm32_imm8(Emulator* emu, ModRM* modrm) {
    // uint32_t rm32 = get_rm32(emu, modrm);
    // uint32_t imm8 = (int32_t) get_sign_code8(emu, 0);
    // emu->rip += 1;
    // uint64_t result = (uint64_t)rm32 - (uint64_t)imm8;
    // update_rflags_sub(emu, rm32, imm8, result);
}

static void code_83(Emulator* emu) {
    // emu->rip += 1;
    // ModRM modrm;
    // parse_modrm(emu, &modrm);

    // switch (modrm.opecode) {
    //     case 0:
    //         add_rm32_imm8(emu, &modrm);
    //         break;
    //     case 5:
    //         sub_rm32_imm8(emu, &modrm);
    //         break;
    //     case 7:
    //         cmp_rm32_imm8(emu, &modrm);
    //         break;
    //     default:
    //         printf("not implemented: 83 /%d\n", modrm.opecode);
    //         exit(1);
    // }
}

static void inc_rm32(Emulator* emu, ModRM* modrm) {
    // uint32_t value = get_rm32(emu, modrm);
    // set_rm32(emu, modrm, value + 1);
}

static void code_0f(Emulator* emu) {
    uint8_t po = get_code8(emu, 1);
    uint8_t oprand = get_code8(emu, 2);
    emu->rip += 3;

    // TODO: What 'oprand & 0xF0' means?
    uint8_t reg = oprand & 0x0F;

    switch (po) {
        case 0x94:
            // 0F 94 C0 => sete al
            set_register8(emu, reg, is_zero(emu));
            break;
        case 0x95:
            // 0F 95 C0 => setne al
            set_register8(emu, reg, !is_zero(emu));
            break;
        case 0x9C:
            // 0F 9C C0 => setl al
            set_register8(emu, reg, is_sign(emu));
            break;
        case 0x9E:
            // 0F 9E C0 => setle al
            set_register8(emu, reg, is_sign(emu) || is_zero(emu));
            break;
        default:
            printf("not implemented: 0F /%d\n", po);
            exit(1);
    }
}

static void code_ff(Emulator* emu) {
    // emu->rip += 1;
    // ModRM modrm;
    // parse_modrm(emu, &modrm);

    // switch (modrm.opecode) {
    //     case 0:
    //         inc_rm32(emu, &modrm);
    //         break;
    //     default:
    //         printf("not implemented: FF /%d\n", modrm.opecode);
    //         exit(1);
    // }
}

#define DEFINE_JX(flag, is_flag) \
static void j ## flag(Emulator *emu) {  \
    int diff = is_flag(emu) ? get_sign_code8(emu, 1) : 0; \
    emu->rip += (diff + 2); \
} \
static void jn ## flag(Emulator *emu) { \
    int diff = is_flag(emu) ? 0 : get_sign_code8(emu, 1); \
    emu->rip += (diff + 2); \
}

DEFINE_JX(c, is_carry)
DEFINE_JX(z, is_zero)
DEFINE_JX(s, is_sign)
DEFINE_JX(o, is_overflow)

static void jl(Emulator* emu) {
    // int diff = (is_sign(emu) != is_overflow(emu)) ? get_sign_code8(emu, 1) : 0;
    // emu->rip += (diff + 2);
}

static void jle(Emulator* emu) {
    // int diff = ( is_zero(emu) || is_sign(emu) != is_overflow(emu)) ? get_sign_code8(emu, 1) : 0;
    // emu->rip += (diff + 2);
}

void short_jump(Emulator *emu) {
    // int8_t diff = get_sign_code8(emu, 1);
    // emu->rip += (diff + 2);
}

void near_jump(Emulator *emu) {
    int32_t diff = get_sign_code32(emu, 1);
    emu->rip += diff + 5;  // diff + oprand(1 byte) + opcode(4 bytes)
}

static void rex_prefix(Emulator* emu) {
    uint8_t wrxb = get_code8(emu, 0) - 0x40;
    emu->rip += 1;
    uint8_t w = (wrxb & 0x08) >> 3;
    uint8_t r = (wrxb & 0x04) >> 2;
    // uint16_t x = (wrxb & 0x02) >> 1;
    uint16_t b = (wrxb & 0x01);

    if (!w) {
        // 32-bit mode
        uint8_t opcode32 = get_code8(emu, 0);
        if (opcode32 >= 0x50 && opcode32 < 0x58) {
            // 41 54 => push r12
            uint8_t reg = get_code8(emu, 0) - 0x50 + R8;
            push64(emu, get_register64(emu, reg));
            emu->rip += 1;
        } else if (opcode32 >= 0x58 && opcode32 <= 0x5F) {
            // 41 5C => pop r12
            uint8_t reg = get_code8(emu, 0) - 0x58 + R8;
            uint64_t value = pop64(emu);
            set_register64(emu, reg, value);
            emu->rip += 1;
        } else if (opcode32 >= 0xB8 && opcode32 <= 0xBF ) {
            // mov_r64_imm32
            // 41 BA 00 00 00 00 => mov r10, 0x0
            uint8_t reg = get_code8(emu, 0) - 0xB8 + R8;
            uint64_t value = get_code32(emu, 1);
            emu->registers[reg] = value;
            emu->rip += 5;  // opcode 1 byte, operand 4 bytes
        } else {
            printf("not implemented: rex_prefix=%02x / w=0\n", 0x40 + wrxb);
            exit(1);
        }
        return;
    }

    // 64-bit mode
    uint16_t po = get_code8(emu, 0); // primary opcode
    emu->rip += 1;

    // Primary opcode only
    if (po == 0x99) {
        // 48 99 => cqo
        // TODO: Must expand rax value to 128 register (RDX, RAX)
        // uint64_t rax = get_register64(emu, RAX);
        // set_register64(emu, RAX, rax & 0x7FFFFFFF);
        // set_register64(emu, RDX, (rax & 0x8FFFFFFF) >> 63);
        set_register64(emu, RDX, 0);
        return;
    }

    // Primary opcode + Secondary opcode (No ModR/M)
    uint8_t so = get_code8(emu, 0);
    if (po == 0x0F && so == 0xAF) { // Signed multiply
        uint8_t oprand = get_code8(emu, 1);
        emu->rip += 2;

        // 0000 1111 : 1010 1111 : 11 : reg1 reg2
        // ex)
        //   48 0F AF C7 => imul rax, rdi  // reg1=0, reg2=7
        //     48 => 0100 1000 => W=1, R=0, X=0, B=0
        //     C7 => 1100 0111 => reg1 = 000 = 0, reg2 = 111 = 7
        //
        //   4D 0F AF D3 => imul r10, r11  // reg1=10, reg2=11
        //     4D => 0100 1101 => W=1, R=1, X=0, B=1
        //     D3 => 1101 0011 => reg1 = 010 = 2, reg2 = 011 = 3
        //
        //   49 0F AF C3 => imul rax,r11   // reg1=0, reg2=11
        //     49 => 0100 1001 => W=1, R=0, X=0, B=1
        //     C3 => 1100 0011 => reg1 = 000 = 0, reg2 = 011 = 3
        //
        //   4C 0F AF D7 => imul r10,rdi  // reg1=10, reg2=7
        //     4C => 0100 1100 => W=1, R=1, X=0, B=0
        //     D7 => 1101 0111 => reg1 = 010 = 2, reg2 = 111 = 7
        uint8_t reg1 = (r << 3) | ((oprand & 0x38) >> 3); // 0011 1000
        uint8_t reg2 = (b << 3) | (oprand & 0x07);  // 0000 B000 | (oprand & 0000 0111)

        // TODO: set overflow values into RDX and set OF=1.
        uint64_t result = get_register64(emu, reg1) * get_register64(emu, reg2);
        set_register64(emu, reg1, result);
        return;
    } else if (po == 0x0F && so == 0xB6) {
        // ex) 48 0F B6 C0 => movzx rax, al
        uint8_t oprand = get_code8(emu, 1);
        emu->rip += 2;
        uint8_t reg = oprand & 0x0F;
        uint64_t result = get_register8(emu, reg);
        set_register64(emu, RAX, result);
        return;
    } else if (po == 0x0F) {
        printf("not implemented: rex_prefix=%02x / w=1 rex_opcode=%02x%02x\n",
               0x40 + wrxb, po, so);
        exit(1);
    }

    // Primary opcode + ModR/M
    ModRM modrm;
    parse_modrm(emu, &modrm);
    uint8_t reg = (r << 3) | modrm.reg_index;
    // mod = modrm.mod;
    uint8_t rm = (b << 3) | modrm.rm;
    // scale = modrm.scale;
    // index = (x << 3) | modrm.index;
    // base = (b << 3) | modrm.base;
    if (po == 0x01) {
        // 48 01 F8 => add rax, rdi
        // 4D 01 E3 => add r11,r12
        uint64_t v1 = get_register64(emu, rm);
        uint64_t v2 = get_register64(emu, reg);
        set_register64(emu, rm, v1 + v2);
    } else if (po == 0x29) {
        // 48 29 F8 => sub rax, rdi
        uint64_t v1 = get_register64(emu, rm);
        uint64_t v2 = get_register64(emu, reg);
        set_register64(emu, rm, v1 - v2);
    } else if (po == 0x39) {
        // 48 39 F8 => cmp rax, rdi
        uint64_t v1 = get_register64(emu, rm);
        uint64_t v2 = get_register64(emu, reg);
        int is_carry = 0; // TODO: Please fix here.
        update_rflags_sub(emu, v1, v2, v1-v2, is_carry);
    } else if (po == 0x89) {
        // 48 89 C8 => sub rax, rdi
        uint64_t value = get_register64(emu, reg);
        set_register64(emu, rm, value);
    } else if (po == 0xF7) {  // Signed Divide
        // 48 F7 FF => idiv rdi(7)
        //   B=0
        //   ModRM = FF
        //     mod(11)=3
        //     reg(111)=7
        //     rm(111)=7
        // 49 F7 FB => idiv r11(11)
        //   B=1
        //   ModRM = FB  1111 1011
        //     mod(11)=3
        //     reg(111)=7
        //     rm(011)=3
        // TODO: Must calculate "(RDX, RAX) / rm64"
        // uint64_t v1h = get_register64(emu, RDX);
        int64_t v1l = get_register64(emu, RAX);
        int64_t v2 = get_register64(emu, rm);

        uint64_t q = v1l / v2;
        uint64_t rem = v1l % v2;
        set_register64(emu, RAX, q);
        set_register64(emu, RDX, rem);
    } else {
        printf("not implemented: rex_prefix=%02x / w=1 rex_opcode=%02x\n",
               0x40 + wrxb, po);
        exit(1);
    }
}

static void push_r64(Emulator *emu) {
    uint8_t reg = get_code8(emu, 0) - 0x50;
    push64(emu, get_register64(emu, reg));
    emu->rip += 1;
}

static void push_imm32(Emulator *emu) {
    // uint32_t value = get_code32(emu, 1);
    // push32(emu, value);
    // emu->rip += 5;
}

static void push_imm8(Emulator *emu) {
    uint8_t value = get_code8(emu, 1);
    push64(emu, value);
    emu->rip += 2;
}

static void pop_r64(Emulator* emu) {
    uint8_t reg = get_code8(emu, 0) - 0x58;
    set_register64(emu, reg, pop64(emu));
    emu->rip += 1;
}

static void call_rel32(Emulator* emu) {
    int32_t diff = get_sign_code32(emu, 1);
    push32(emu, emu->rip + 5);
    emu->rip += (diff + 5);  // jump
}

static void neg(Emulator* emu) {
    //emu->rip += 1; // opcode
    //ModRM modrm;
    //parse_modrm(emu, &modrm);
    //int32_t rm32 = (int32_t) get_rm32(emu, &modrm);
    //set_rm32(emu, &modrm, (uint32_t) -rm32);
}

static void nop(Emulator* emu) {
    emu->rip += 1;
}

static void ret(Emulator* emu) {
    emu->rip = pop32(emu);
}

static void leave(Emulator* emu) {
    // emu->rip += 1;
    // set_register32(emu, ESP, get_register32(emu, EBP));
    // set_register32(emu, EBP, pop32(emu));
}

static void setz_al(Emulator* emu) {
    uint16_t address = get_register32(emu, RDX) & 0xffff;
    // uint8_t value = io_in8(address);
    // set_register8(emu, AL, value);
    // emu->rip += 1;
}

static void in_al_dx(Emulator* emu) {
    // uint16_t address = get_register32(emu, EDX) & 0xffff;
    // uint8_t value = io_in8(address);
    // set_register8(emu, AL, value);
    // emu->rip += 1;
}

static void out_dx_al(Emulator* emu) {
    // uint16_t address = get_register32(emu, EDX) & 0xffff;
    // uint8_t value = get_register8(emu, AL);
    // io_out8(address, value);
    // emu->rip += 1;
}

void init_instructions(void) {
    int i;
    memset(instructions, 0, sizeof(instructions));

    instructions[0x01] = add_rm32_r32;
    instructions[0x0F] = code_0f;
    instructions[0x3B] = cmp_r32_rm32;
    instructions[0x3C] = cmp_al_imm8;

    // REX prefixes are a set of 16 opcodes that span one row of the opcode map and occupy entries 40H to 4FH.
    for (i=0; i<16; i++) {
        instructions[0x40+i] = rex_prefix;
    }

    for (i=0; i<8; i++) {
        instructions[0x50 + i] = push_r64;
    }
    for (i=0; i<8; i++) {
        instructions[0x58 + i] = pop_r64;
    }

    instructions[0x68] = push_imm32;
    instructions[0x6a] = push_imm8;

    instructions[0x70] = jo;
    instructions[0x71] = jno;
    instructions[0x72] = jc;
    instructions[0x73] = jnc;
    instructions[0x74] = jz;
    instructions[0x75] = jnz;
    instructions[0x78] = js;
    instructions[0x79] = jns;
    instructions[0x7C] = jl;
    instructions[0x7E] = jle;

    instructions[0x83] = code_83;
    instructions[0x88] = mov_rm8_r8;
    instructions[0x89] = mov_rm32_r32;
    instructions[0x8a] = mov_r8_rm8;
    instructions[0x8B] = mov_r32_rm32;
    instructions[0x90] = nop;

    for (i=0; i<8; i++) {
        instructions[0xB0+i] = mov_r8_imm8;
    }

    for (i=0; i<8; i++) {
        instructions[0xB8 + i] = mov_r32_imm32;
    }
    instructions[0xC3] = ret;
    instructions[0xC7] = mov_rm32_imm32;
    instructions[0xC9] = leave;
    instructions[0xE8] = call_rel32;
    instructions[0xE9] = near_jump;
    instructions[0xEB] = short_jump;
    instructions[0xEC] = in_al_dx;
    instructions[0xEE] = out_dx_al;
    instructions[0xF7] = neg;
    instructions[0xFF] = code_ff;
}
