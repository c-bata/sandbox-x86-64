#include "emulator_function.h"

uint8_t get_code8(Emulator* emu, int index) {
    return emu->memory[emu->rip + index];
}

int8_t get_sign_code8(Emulator* emu, int index) {
    return (int8_t)emu->memory[emu->rip + index];
}

uint32_t get_code32(Emulator* emu, int index) {
    int i;
    uint32_t ret = 0;

    // little endian
    for (i=0;i<4; i++) {
        ret |= get_code8(emu, index+i) << (i * 8);
    }
    return ret;
}

int32_t get_sign_code32(Emulator* emu, int index) {
    return (int32_t) get_code32(emu, index);
}

void set_memory8(Emulator* emu, uint32_t address, uint32_t value) {
    emu->memory[address] = value & 0xFF;
}

void set_memory32(Emulator* emu, uint32_t address, uint32_t value) {
    int i;
    for (i = 0; i<4; i++) {
        set_memory8(emu, address+i, value >> (i*8));
    }
}

uint32_t get_memory8(Emulator* emu, uint32_t address) {
    return (uint32_t) emu->memory[address];
}

uint32_t get_memory32(Emulator* emu, uint32_t address) {
    int i;
    uint32_t ret = 0;
    for (i=0; i<4; i++) {
        ret |= get_memory8(emu, address+i) << (i*8);
    }
    return ret;
}

uint32_t get_register32(Emulator* emu, int index) {
    return emu->registers[index];
}

/* 31                              0
 * |--------------------------------|
 * |               eax              |
 * |--------------------------------|
 *                15               0
 *                 |----------------|
 *                 |       ax       |
 *                 |----------------|
 *                15       8
 *                 |--------|
 *                 |   ah   |
 *                 |--------|
 *                          7      0
 *                         |--------|
 *                         |   al   |
 *                         |--------|
 * */
uint8_t get_register8(Emulator* emu, int index) {
    if (index < 4) { // al, cl, dl, bl
        return emu->registers[index] & 0xff;
    } else { // ah, ch, dh, bh
        return (emu->registers[index - 4] >> 8) & 0xff;
    }
}

void set_register8(Emulator* emu, int index, uint8_t value) {
    if (index < 4) {
        uint32_t r = emu->registers[index] & 0xffffff00;
        emu->registers[index] = r | (uint32_t)value;
    } else {
        uint32_t r = emu->registers[index] & 0xffff00ff;
        emu->registers[index-4] = r | ((uint32_t)value << 8);
    }
}

void set_register32(Emulator* emu, int index, uint32_t value) {
    emu->registers[index] = value;
}

void push32(Emulator* emu, uint32_t value) {
    uint32_t  address = get_register32(emu, RSP) - 4;
    set_register32(emu, RSP, address);
    set_memory32(emu, address, value);
}

uint32_t pop32(Emulator* emu) {
    uint32_t address = get_register32(emu, RSP);
    uint32_t ret = get_memory32(emu, address);
    set_register32(emu, RSP, address + 4);
    return ret;
}

void set_carry(Emulator* emu, int is_carry) {
    if (is_carry) {
        emu->rflags |= CARRY_FLAG;
    } else {
        emu->rflags &= ~CARRY_FLAG;
    }
}

void set_zero(Emulator* emu, int is_zero) {
    if (is_zero) {
        emu->rflags |= ZERO_FLAG;
    } else {
        emu->rflags &= ~ZERO_FLAG;
    }
}

void set_sign(Emulator* emu, int is_sign) {
    if (is_sign) {
        emu->rflags |= SIGN_FLAG;
    } else {
        emu->rflags &= ~SIGN_FLAG;
    }
}

void set_overflow(Emulator* emu, int is_overflow) {
    if (is_overflow) {
        emu->rflags |= OVERFLOW_FLAG;
    } else {
        emu->rflags &= ~OVERFLOW_FLAG;
    }
}

void update_rflags_sub(Emulator* emu, uint32_t v1, uint32_t v2, uint64_t result) {
    int sign1 = v1 >> 31;
    int sign2 = v2 >> 31;
    int signr = (result >> 31) & 1;
    set_carry(emu, (result >> 32) != 0); // 単に (result >> 32) でも可
    set_zero(emu, result==0);
    set_sign(emu, signr);
    set_overflow(emu, sign1 != sign2 && sign1 != signr);
}

int32_t is_carry(Emulator* emu) {
    return (emu->rflags & CARRY_FLAG) != 0;
}

int32_t is_zero(Emulator* emu) {
    return (emu->rflags & ZERO_FLAG) != 0;
}

int32_t is_sign(Emulator* emu) {
    return (emu->rflags & SIGN_FLAG) != 0;
}
int32_t is_overflow(Emulator* emu) {
    return (emu->rflags & OVERFLOW_FLAG) != 0;
}
