#ifndef EMULATOR_H_
#define EMULATOR_H_

#include <stdint.h>

enum Register {
    RAX, RCX, RDX, RBX, RSP, RBP, RSI, RDI,
    R8, R9, R10, R11, R12, R13, R14, R15,
    REGISTERS_COUNT,
    AL = RAX, CL = RCX, DL = RDX, BL = RBX,
    AH = AL + 4, CH = CL + 4, DH = DL + 4, BH = BL + 4};

typedef struct {
    uint64_t registers[REGISTERS_COUNT];
    uint64_t rflags;
    uint8_t* memory;  // Memory (byte array)
    uint64_t rip;
} Emulator;

#endif