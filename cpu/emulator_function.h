#ifndef EMULATOR_FUNCTION_H_
#define EMULATOR_FUNCTION_H_

#include <stdint.h>

#include "emulator.h"

#define CARRY_FLAG (1)
#define ZERO_FLAG (1 << 6)
#define SIGN_FLAG (1 << 7)
#define OVERFLOW_FLAG (1 << 11)

Emulator* create_emu(uint64_t rip, uint64_t rsp);
void destroy_emu(Emulator* emu);

uint8_t get_code8(Emulator* emu, int index);
int8_t get_sign_code8(Emulator* emu, int index);
uint32_t get_code32(Emulator* emu, int index);
int32_t get_sign_code32(Emulator* emu, int index);

void set_memory8(Emulator* emu, uint64_t address, uint64_t value);
void set_memory32(Emulator* emu, uint64_t address, uint64_t value);
void set_memory64(Emulator* emu, uint64_t address, uint64_t value);
uint64_t get_memory8(Emulator* emu, uint64_t address);
uint64_t get_memory32(Emulator* emu, uint64_t address);
uint64_t get_memory64(Emulator* emu, uint64_t address);

uint64_t get_register8(Emulator* emu, int index);
uint64_t get_register32(Emulator* emu, int index);
uint64_t get_register64(Emulator* emu, int index);

void set_register8(Emulator* emu, int index, uint8_t value);
void set_register32(Emulator* emu, int index, uint32_t value);
void set_register64(Emulator* emu, int index, uint64_t value);

void push32(Emulator* emu, uint32_t value);
uint32_t pop32(Emulator* emu);
void push64(Emulator* emu, uint64_t value);
uint64_t pop64(Emulator* emu);

void update_rflags_sub(Emulator* emu, uint64_t v1, uint64_t v2, uint64_t result, int is_carry);
int carry_flag_add(uint64_t v1, uint64_t v2);
int carry_flag_sub(uint64_t v1, uint64_t v2);

int is_carry(Emulator* emu);
int is_zero(Emulator* emu);
int is_sign(Emulator* emu);
int is_overflow(Emulator* emu);

#endif