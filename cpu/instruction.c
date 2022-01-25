#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "instruction.h"
#include "emulator.h"
#include "emulator_function.h"
#include "modrm.h"

instruction_func_t* instructions[256];

static void mov_r8_imm8(Emulator* emu) {
    uint8_t reg= get_code8(emu, 0) - 0xB0;
    uint8_t imm8 = get_code8(emu, 1);
    set_register8(emu, reg, imm8);
    emu->rip += 2;
}

static void mov_r32_imm32(Emulator* emu) {
    uint8_t reg = get_code8(emu, 0) - 0xB8;
    uint64_t value = get_code32(emu, 1);
    emu->registers[reg] = value;
    emu->rip += 5;  // opcode 1 byte, operand 4 bytes
}

static void mov_rm32_imm32(Emulator* emu) {
    fprintf(stderr, "CPU Warning: mov_rm32_imm32 may be wrong behavior.\n");
    emu->rip += 1;
    ModRM modrm;
    parse_modrm(emu, &modrm);
    uint32_t value = get_code32(emu, 0);
    emu->rip += 4;  // operand 4 bytes
    set_rm32(emu, &modrm, value);
}

static void mov_r32_rm32(Emulator* emu) {
    emu->rip += 1;
    ModRM modrm;
    parse_modrm(emu, &modrm);
    uint32_t r32 = get_rm32(emu, &modrm);
    set_r32(emu, &modrm, r32);
}

static void mov_rm32_r32(Emulator* emu) {
    emu->rip += 1;
    ModRM modrm;
    parse_modrm(emu, &modrm);
    uint32_t r32 = get_r32(emu, &modrm);
    set_rm32(emu, &modrm, r32);
}

static void mov_rm8_r8(Emulator* emu) {
    emu->rip += 1;
    ModRM modrm;
    parse_modrm(emu, &modrm);
    uint8_t r8 = get_r8(emu, &modrm);
    set_rm8(emu, &modrm, r8);
}

static void mov_r8_rm8(Emulator* emu) {
    emu->rip += 1;
    ModRM modrm;
    parse_modrm(emu, &modrm);
    uint8_t rm8 = get_rm8(emu, &modrm);
    set_r8(emu, &modrm, rm8);
}

static void add_rm32_r32(Emulator* emu) {
    emu->rip += 1;
    ModRM modrm;
    parse_modrm(emu, &modrm);
    uint32_t r32 = get_r32(emu, &modrm);
    uint32_t rm32 = get_rm32(emu, &modrm);
    set_rm32(emu, &modrm, rm32 + r32);
}

static void add_r32_rm32(Emulator* emu) {
    // 03 45 f8 => addl -0x8(%rbp), %eax
    emu->rip += 1;
    ModRM modrm;
    parse_modrm(emu, &modrm);
    uint32_t r32 = get_r32(emu, &modrm);
    uint32_t rm32 = get_rm32(emu, &modrm);
    set_r32(emu, &modrm, rm32 + r32);
}

static void sub_r32_rm32(Emulator* emu) {
    // 2b 45 f8 => subl -0x8(%rbp), %eax
    emu->rip += 1;
    ModRM modrm;
    parse_modrm(emu, &modrm);
    uint32_t r32 = get_r32(emu, &modrm);
    uint32_t rm32 = get_rm32(emu, &modrm);
    set_r32(emu, &modrm, r32 - rm32);
}

static void xor_rm32_r32(Emulator* emu) {
    // 31 db => xor ebx,ebx
    emu->rip += 1;
    ModRM modrm;
    parse_modrm(emu, &modrm);
    uint32_t r32 = get_r32(emu, &modrm);
    uint32_t rm32 = get_rm32(emu, &modrm);
    set_rm32(emu, &modrm, r32 ^ rm32);  // set_r32?
    fprintf(stderr, "CPU Warning: xor_rm32_r32 may be wrong behavior.\n");
}

static void cmp_r32_rm32(Emulator* emu) {
    fprintf(stderr, "CPU Warning: cmp_r32_rm32 may be wrong behavior.\n");
    emu->rip += 1;
    ModRM modrm;
    parse_modrm(emu, &modrm);
    uint32_t r32 = get_r32(emu, &modrm);
    uint32_t rm32 = get_rm32(emu, &modrm);
    uint64_t result = (uint64_t) r32 - (uint64_t) rm32;
    int is_carry = carry_flag_sub(r32, rm32);
    update_rflags_sub(emu, r32, rm32, result, is_carry);
}

static void cmp_al_imm8(Emulator* emu) {
    fprintf(stderr, "CPU Warning: cmp_r32_rm32 may be wrong behavior.\n");
    uint8_t al = get_register8(emu, AL);
    uint8_t imm8 = get_code8(emu, 1);
    uint64_t result = (uint64_t) al - (uint64_t) imm8;
    int is_carry = carry_flag_sub(al, imm8);
    update_rflags_sub(emu, al, imm8, result, is_carry);
    emu->rip += 2;
}

static void add_rm32_imm8(Emulator* emu, ModRM* modrm) {
    uint32_t rm32 = get_rm32(emu, modrm);
    uint32_t imm8 = (int32_t)get_sign_code8(emu, 0);
    emu->rip += 1;
    set_rm32(emu, modrm, rm32 + imm8);
}

static void sub_rm32_imm8(Emulator* emu, ModRM* modrm) {
    uint32_t rm32 = get_rm32(emu, modrm);
    uint32_t imm8 = (int32_t) get_sign_code8(emu, 0);
    emu->rip += 1;
    uint64_t result = (uint64_t)rm32 - (uint64_t)imm8;
    set_rm32(emu, modrm, (uint32_t) result);
    int is_carry = carry_flag_sub(rm32, imm8);
    update_rflags_sub(emu, rm32, imm8, result, is_carry);
}

static void cmp_rm32_imm8(Emulator* emu, ModRM* modrm) {
    uint32_t rm32 = get_rm32(emu, modrm);
    uint32_t imm8 = (int32_t) get_sign_code8(emu, 0);
    emu->rip += 1;
    uint64_t result = (uint64_t)rm32 - (uint64_t)imm8;
    int is_carry = carry_flag_sub(rm32, imm8);
    update_rflags_sub(emu, rm32, imm8, result, is_carry);
}

static void code_83(Emulator* emu) {
    fprintf(stderr, "CPU Warning: code_83 may be wrong behavior.\n");
    emu->rip += 1;
    ModRM modrm;
    parse_modrm(emu, &modrm);

    switch (modrm.opecode) {
        case 0:
            add_rm32_imm8(emu, &modrm);
            break;
        case 5:
            sub_rm32_imm8(emu, &modrm);
            break;
        case 7:
            cmp_rm32_imm8(emu, &modrm);
            break;
        default:
            printf("not implemented: 83 /%d\n", modrm.opecode);
            exit(1);
    }
}

static void code_0f(Emulator* emu) {
    uint8_t po = get_code8(emu, 1);
    if (po == 0x94 || po == 0x95 || po == 0x9C || po == 0x9E) {
        uint8_t oprand = get_code8(emu, 2);
        emu->rip += 3;

        // TODO: Check What 'oprand & 0xF0' means?
        uint8_t reg = oprand & 0x0F;

        switch (po) {
            case 0x94:
                // 0F 94 C0 => sete al
                set_register8(emu, reg, is_zero(emu));
                return;
            case 0x95:
                // 0F 95 C0 => setne al
                set_register8(emu, reg, !is_zero(emu));
                return;
            case 0x9C:
                // 0F 9C C0 => setl al
                set_register8(emu, reg, is_sign(emu));
                return;
            case 0x9E:
                // 0F 9E C0 => setle al
                set_register8(emu, reg, is_sign(emu) || is_zero(emu));
                return;
        }
    } else if (po >= 0x80 && po <= 0x8E) {
        int diff;
        switch (po) {
            case 0x80:
                diff = is_overflow(emu) ? get_sign_code32(emu, 2) : 0;
                break;
            case 0x81:
                diff = !is_overflow(emu) ? get_sign_code32(emu, 2) : 0;
                break;
            case 0x82:
                diff = is_carry(emu) ? get_sign_code32(emu, 2) : 0;
                break;
            case 0x83:
                diff = !is_carry(emu) ? get_sign_code32(emu, 2) : 0;
                break;
            case 0x84:
                // 0f 84 0c 00 00 00 => je 0x0c
                diff = is_zero(emu) ? get_sign_code32(emu, 2) : 0;
                break;
            case 0x85:
                diff = !is_zero(emu) ? get_sign_code32(emu, 2) : 0;
                break;
            case 0x88:
                diff = is_sign(emu) ? get_sign_code32(emu, 2) : 0;
                break;
            case 0x89:
                diff = !is_sign(emu) ? get_sign_code32(emu, 2) : 0;
                break;
            default:
                printf("not implemented: 0F /%d\n", po);
                exit(1);
        }
        emu->rip += (diff + 6);
        return;
    } else {
        printf("not implemented: 0F /%d\n", po);
        exit(1);
    }
}

static void code_fe(Emulator* emu) {
    emu->rip += 1;
    ModRM modrm;
    parse_modrm(emu, &modrm);

    // See Table A.6, Volume 2D A-18.
    switch (modrm.opecode) {
        case 0:  // INC Eb
        case 1:  // INC Eb
        default:
            printf("not implemented: FE, modrm.opecode=%d\n", modrm.opecode);
            exit(1);
    }
}

static void code_ff(Emulator* emu) {
    emu->rip += 1;
    ModRM modrm;
    parse_modrm(emu, &modrm);

    // See Table A.6, Volume 2D A-18.
    switch (modrm.opecode) {
        case 0: {
            // INC rm32
            uint32_t value = get_rm32(emu, &modrm);
            set_rm32(emu, &modrm, value + 1);
            break;
        }
        case 1: {
            // DEC rm32
            uint32_t value = get_rm32(emu, &modrm);
            set_rm32(emu, &modrm, value - 1);
            break;
        }
        case 2: {
            // near CALL Ev
            // ex) ff 15 72 2f 00 00 => call QWORD PTR [rip+0x2f72]
            //     rm = 5 (= RSP) but unused.
            //     disp32 = 0x2f72;
            push32(emu, emu->rip);
            emu->rip += ((int32_t) modrm.disp32);
            break;
        }
        case 3: // far CALL Ep
        case 4: // near JMP Ev
        case 5: // far JMP Mp
        case 6: // PUSH Ev
        default:
            printf("not implemented: FF /%d\n", modrm.opecode);
            exit(1);
    }
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
    int diff = (is_sign(emu) != is_overflow(emu)) ? get_sign_code8(emu, 1) : 0;
    emu->rip += (diff + 2);
}

static void jle(Emulator* emu) {
    int diff = ( is_zero(emu) || is_sign(emu) != is_overflow(emu)) ? get_sign_code8(emu, 1) : 0;
    emu->rip += (diff + 2);
}

void short_jump(Emulator *emu) {
    int8_t diff = get_sign_code8(emu, 1);
    emu->rip += (diff + 2);
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
        } else if (opcode32 == 0x89) {
            // mov_rm32_r32
            // 44 89 45 ec => movl %r8d, -0x14(%rbp)
            emu->rip += 1;
            ModRM modrm;
            parse_modrm(emu, &modrm);

            uint8_t reg = (r << 3) | (modrm.reg_index + R8);
            uint32_t r32 = get_register64(emu, reg);
            set_rm32(emu, &modrm, r32);  // TODO(c-bata): We may be need to implement set_rm64 here.
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
        uint8_t oprand = get_code8(emu, 1);
        emu->rip += 2;
        // ex) 48 0F B6 C0 => movzx rax, al
        //     48 => 0100 1000 => W=1, R=0, X=0, B=0
        //     C0 => 1100 0000 => reg1 = 000 = 0, reg2 = 000 = 0
        // ex) 4C 0F B6 D0 => movzx r10, al
        //     4C => 0100 1100 => W=1, R=1, X=0, B=0
        //     D0 => 1101 0000 => reg1 = 000 = 0, reg2 = 000 = 0
        uint8_t reg1 = (r << 3) | ((oprand & 0x38) >> 3); // 0011 1000
        uint8_t reg2 = (b << 3) | (oprand & 0x07);  // 0000 B000 | (oprand & 0000 0111)

        uint64_t result = get_register8(emu, reg2);
        set_register64(emu, reg1, result);
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

        int is_carry = carry_flag_add(v1, v2);
        update_rflags_sub(emu, v1, v2, v1 + v2, is_carry);
    } else if (po == 0x29) {
        // 48 29 F8 => sub rax, rdi
        uint64_t v1 = get_register64(emu, rm);
        uint64_t v2 = get_register64(emu, reg);
        set_register64(emu, rm, v1 - v2);

        int is_carry = carry_flag_sub(v1, v2);
        update_rflags_sub(emu, v1, v2, v1 - v2, is_carry);
    } else if (po == 0x39) {
        // 48 39 F8 => cmp rax, rdi
        uint64_t v1 = get_register64(emu, rm);
        uint64_t v2 = get_register64(emu, reg);

        int is_carry = carry_flag_sub(v1, v2);
        update_rflags_sub(emu, v1, v2, v1 - v2, is_carry);
    } else if (po == 0x81) {
        // SUB: immediate -> register
        // 1000 00sw : 11 101 reg : immediate data
        // ex) 48 81 EC D0 00 00 00 => sub rsp,0xd0
        uint64_t v2 = get_code32(emu, 0);
        uint64_t v1 = get_register64(emu, rm);
        set_register64(emu, rm, v1 - v2);
        emu->rip += 4;

        int is_carry = carry_flag_sub(v1, v2);
        update_rflags_sub(emu, v1, v2, v1 - v2, is_carry);
    // } else if (po == 0x83 && modrm.mod == 0 && modrm.opecode == 7) {
    //     // ex) 48 83 3d 02 2f 00 00 => cmp QWORD PTR [rip+0x2f02],0x0
    } else if (po == 0x83 && modrm.mod == 3 && modrm.opecode == 4) {
        // 48 83 e4 f0 => and rsp,0xf0
        uint8_t imm8 = get_code8(emu, 0);
        emu->rip += 1;
        uint64_t value = get_register64(emu, rm) & imm8;
        set_register64(emu, rm, value);
    } else if (po == 0x83 && modrm.mod == 3 && modrm.opecode == 5) {
        // 1000 00sw : 11 101 reg : immediate data
        // ex) 48 83 EC 00 => sub rsp,byte +0x0
        int8_t imm8 = get_sign_code8(emu, 0);
        emu->rip += 1;
        int64_t val = get_register64(emu, rm);
        set_register64(emu, rm, val-imm8);

        int is_carry = carry_flag_sub(val, imm8);
        update_rflags_sub(emu, val, imm8, val - imm8, is_carry);
    } else if (po == 0x83 && modrm.mod == 3 && modrm.opecode == 7) {
        // 1000 00sw : 11 111 reg : immediate data
        // ex) 48 83 F8 00 => cmp rax,byte +0x0
        int64_t imm8 = (int64_t) get_sign_code8(emu, 0);
        emu->rip += 1;
        int64_t val = (int64_t) get_register64(emu, rm);
        uint64_t result = val - imm8;

        int is_carry = carry_flag_sub(val, imm8);
        update_rflags_sub(emu, val, imm8, result, is_carry);
    } else if (po == 0x89 && modrm.mod == 0) {
        // ex) 48 89 07 => mov [rdi],rax
        uint64_t addr = get_register64(emu, rm);
        uint64_t val = get_register64(emu, reg);
        set_memory64(emu, addr, val);
    } else if (po == 0x89 && modrm.mod == 1) {
        // 48 89 7d f8 => mov QWORD PTR [rbp-0x8],rdi
        // ModRM=0x7d => 0111 1101 => mod 01, reg 111, rm 101
        uint64_t addr = get_register64(emu, rm) + modrm.disp8;
        uint64_t value = get_register64(emu, reg);
        set_memory64(emu, addr, value);
    } else if (po == 0x89 && modrm.mod == 3) {
        // 48 89 C8 => mov rax, rdi
        uint64_t value = get_register64(emu, reg);
        set_register64(emu, rm, value);
    } else if (po == 0x8B) {
        // 1000 101w : mod reg r/m
        // ex) 48 8B 07 => mov rax,[rdi]
        uint64_t addr = get_register64(emu, rm);
        uint64_t val = get_memory64(emu, addr);
        set_register64(emu, reg, val);
    } else if (po == 0x8D && modrm.mod == 1) {
        // ex) 48 8D 45 F8 => lea rax,[rbp-0x8]
        uint64_t addr = get_register64(emu, rm) + modrm.disp8;
        set_register64(emu, reg, addr);
    } else if (po == 0x8D && ((modrm.mod == 0 && modrm.rm == 5) || modrm.mod == 2)) {
        // ex) 48 8D 85 30FFFFFF => lea rax,[rbp-0xd0]
        uint64_t addr = get_register64(emu, rm) + (int32_t) modrm.disp32;
        set_register64(emu, reg, addr);
    } else if (po == 0x8D) {
        printf("not implemented: rex_prefix=0x8D / mod=%d rm=%d\n", modrm.mod, modrm.rm);
        exit(1);
    } else if (po == 0xC7) {
        // ex) 48 c7 c0 0a 00 00 00 => movq $0xa, %rax
        uint64_t value = get_sign_code32(emu, 0);
        set_register64(emu, rm, value);
        emu->rip += 4;
    } else if (po == 0xF7 && modrm.opecode == 7) {
        // Signed Divide - 1111 011w : 11 111 reg
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
    } else if (po == 0xF7 && modrm.opecode == 3) {
        // Two's Complement Negation - 1111 011w : 11 011 reg
        // 49 F7 DA => code_f7 r10
        int64_t value = get_register64(emu, rm);
        set_register64(emu, rm, -value);
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
    uint32_t value = get_code32(emu, 1);
    push32(emu, value);
    emu->rip += 5;
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

static void endbr64(Emulator* emu) {
    // TODO(c-bata): Implement here. Currently just skips 4 bytes.
    fprintf(stderr, "CPU Warning: endbr64 is skipped.\n");
    emu->rip += 4;
}

static void code_f7(Emulator* emu) {
    emu->rip += 1; // opcode
    ModRM modrm;
    parse_modrm(emu, &modrm);
    switch (modrm.opecode) {
        // Table A-6, Vol. 2D
        case 3: {
            int32_t rm32 = (int32_t) get_rm32(emu, &modrm);
            set_rm32(emu, &modrm, (uint32_t) -rm32);
            return;
        }
        case 0: // TEST
        case 2: // NOT
        case 4: // MUL AL/rAX
        case 5: // IMUL AL/rAX
        case 6: // DIV AL/rAX
        case 7: // IDIV AL/rAX
        default:
            fprintf(stderr, "Not implemented 0xF7 modrm.opecode=%d\n", modrm.opecode);
            exit(EXIT_FAILURE);
    }
}

static void nop(Emulator* emu) {
    emu->rip += 1;
}

static void ret(Emulator* emu) {
    emu->rip = pop32(emu);
}

static void leave(Emulator* emu) {
    fprintf(stderr, "CPU Warning: leave may be wrong behavior.\n");
    emu->rip += 1;
    set_register64(emu, RSP, get_register64(emu, RBP));
    set_register32(emu, RBP, pop32(emu));
}

void init_instructions(void) {
    int i;
    memset(instructions, 0, sizeof(instructions));

    instructions[0x01] = add_rm32_r32;
    instructions[0x03] = add_r32_rm32;
    instructions[0x0F] = code_0f;
    instructions[0x2B] = sub_r32_rm32;
    instructions[0x31] = xor_rm32_r32;
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
    instructions[0xF3] = endbr64;
    instructions[0xF7] = code_f7;
    instructions[0xFE] = code_fe;
    instructions[0xFF] = code_ff;
}
