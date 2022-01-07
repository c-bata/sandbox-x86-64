#ifndef EMULATOR_H_
#define EMULATOR_H_

#include <stdint.h>
#include "virtual_memory.h"

enum Register {
    RAX, RCX, RDX, RBX, RSP, RBP, RSI, RDI,
    R8, R9, R10, R11, R12, R13, R14, R15,
    REGISTERS_COUNT,
    AL = RAX, CL = RCX, DL = RDX, BL = RBX,
    SPL = RSP, BPL = RBP, SIL = RSI, DIL = RDI,
    R8B = R8, R9B = R9, R10B = R10, R11B = R11,
    R12B = R12, R13B = R13, R14B = R14, R15B = R15,
    AH = AL + 4, CH = CL + 4, DH = DL + 4, BH = BL + 4,
    SPH = SPL + 4, BPH = BPL + 4, SIH = SIL + 4, DIH = DIL + 4};

typedef struct {
    uint64_t registers[REGISTERS_COUNT];
    uint64_t rflags;
    VirtualMemory* memory;  // Memory (byte array)
    uint64_t rip;
} Emulator;

#endif