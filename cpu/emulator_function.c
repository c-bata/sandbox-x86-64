#include <string.h>
#include <stdlib.h>
#include "emulator_function.h"
#include "virtual_memory.h"

Emulator* create_emu(uint64_t rip, uint64_t rsp) {
    Emulator* emu = malloc(sizeof(Emulator));
    emu->memory = vm_init();

    memset(emu->registers, 0, sizeof(emu->registers));
    emu->rip = rip;
    emu->registers[RSP] = rsp;
    return emu;
}

void destroy_emu(Emulator* emu) {
    free(emu->memory);
    free(emu);
}

uint8_t get_code8(Emulator* emu, int index) {
    return vm_get_memory8(emu->memory, emu->rip + index);
}

int8_t get_sign_code8(Emulator* emu, int index) {
    return (int8_t)get_code8(emu, index);
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

void set_memory8(Emulator* emu, uint64_t address, uint64_t value) {
    vm_set_memory8(emu->memory, address, value & 0xFF);
}

void set_memory32(Emulator* emu, uint64_t address, uint64_t value) {
    int i;
    for (i = 0; i<4; i++) {
        set_memory8(emu, address+i, value >> (i*8));
    }
}

void set_memory64(Emulator* emu, uint64_t address, uint64_t value) {
    int i;
    for (i = 0; i<8; i++) {
        set_memory8(emu, address+i, value >> (i*8));
    }
}

uint64_t get_memory8(Emulator* emu, uint64_t address) {
    return vm_get_memory8(emu->memory, address);
}

uint64_t get_memory32(Emulator* emu, uint64_t address) {
    int i;
    uint32_t ret = 0;
    for (i=0; i<4; i++) {
        ret |= get_memory8(emu, address+i) << (i*8);
    }
    return ret;
}

uint64_t get_memory64(Emulator* emu, uint64_t address) {
    int i;
    uint64_t ret = 0;
    for (i=0; i<8; i++) {
        ret |= get_memory8(emu, address+i) << (i*8);
    }
    return ret;
}

uint64_t get_register32(Emulator* emu, int index) {
    return emu->registers[index];
}

uint64_t get_register64(Emulator* emu, int index) {
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
uint64_t get_register8(Emulator* emu, int index) {
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

void set_register64(Emulator* emu, int index, uint64_t value) {
    emu->registers[index] = value;
}

// TODO: should be removed.
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

void push64(Emulator* emu, uint64_t value) {
    uint64_t address = get_register64(emu, RSP) - 8;
    set_register64(emu, RSP, address);
    set_memory64(emu, address, value);
}

uint64_t pop64(Emulator* emu) {
    uint64_t address = get_register64(emu, RSP);
    uint64_t ret = get_memory64(emu, address);
    set_register64(emu, RSP, address + 8);
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

int carry_flag_sub(uint64_t v1, uint64_t v2) {
    int sign1 = v1 >> 63;
    int signr = (v1 - v2) >> 63;
    int is_carry = (sign1 == 0) && (signr == 1);
    return is_carry;
}

int carry_flag_add(uint64_t v1, uint64_t v2) {
    int sign1 = v1 >> 63;
    int sign2 = v2 >> 63;
    if (sign1 == 1 && sign2 == 2) {
        return 1;
    }
    int signr = (v1 + v2) >> 63;
    return (sign1 + sign2) >= 1 && (signr == 0);
}

void update_rflags_sub(Emulator* emu, uint64_t v1, uint64_t v2, uint64_t result, int is_carry) {
    int sign1 = v1 >> 63;
    int sign2 = v2 >> 63;
    int signr = (result >> 63) & 1;
    set_carry(emu, is_carry);
    set_zero(emu, result==0);
    set_sign(emu, signr);
    set_overflow(emu, sign1 != sign2 && sign1 != signr);
}

int is_carry(Emulator* emu) {
    return (emu->rflags & CARRY_FLAG) != 0;
}

int is_zero(Emulator* emu) {
    return (emu->rflags & ZERO_FLAG) != 0;
}

int is_sign(Emulator* emu) {
    return (emu->rflags & SIGN_FLAG) != 0;
}
int is_overflow(Emulator* emu) {
    return (emu->rflags & OVERFLOW_FLAG) != 0;
}
