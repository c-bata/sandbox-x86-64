#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdarg.h>
#include <stdbool.h>

#include "emulator_function.h"
#include "instruction.h"

#define MEMORY_SIZE (1024 * 1024)

bool quiet = false;
char* registers_name[] = {"EAX", "ECX", "EDX", "EBX", "ESP", "EBP", "ESI", "EDI"};

void debugf(char *fmt, ...) {
    if (quiet) {
        return;
    }
    va_list args;
    va_start(args, fmt);
    vfprintf(stderr, fmt, args);
}

void errorf(char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    vfprintf(stderr, fmt, args);
    exit(1);
}

Emulator* create_emu(size_t size, uint32_t eip, uint32_t esp) {
    Emulator* emu = malloc(sizeof(Emulator));
    emu->memory = malloc(size);

    memset(emu->registers, 0, sizeof(emu->registers));
    emu->eip = eip;
    emu->registers[ESP] = esp;
    return emu;
}

void destroy_emu(Emulator* emu) {
    free(emu->memory);
    free(emu);
}

static void dump_registers(Emulator* emu) {
    int i;
    for (i = 0; i < REGISTERS_COUNT; i++) {
        debugf("%s = %08x\n", registers_name[i], emu->registers[i]);
    }
    debugf("EIP = %08x\n", emu->eip);
}

int opt_remove_at(int argc, char* argv[], int index) {
    if (index < 0 || argc <= index) {
        return argc;
    } else {
        int i = index;
        for (; i<argc-1; i++) {
            argv[i] = argv[i+1];
        }
        argv[i] = NULL;
        return argc -1;
    }
}

int main(int argc, char* argv[]) {
    FILE* binary;
    Emulator* emu;
    int i = 1;
    while (i < argc) {
        if (strcmp(argv[i], "-q") == 0 || strcmp(argv[i], "--quiet") == 0) {
            quiet = true;
            argc = opt_remove_at(argc, argv, i);
        } else {
            i++;
        }
    }

    emu = create_emu(MEMORY_SIZE, 0x7c00, 0x7c00);

    binary = fopen(argv[1], "rb");
    if (binary == NULL) {
        errorf("cannot open file '%s'\n", argv[1]);
    }

    // read machine program (max 512 bytes)
    fread(emu->memory + 0x7c00, 1, 0x200, binary);
    fclose(binary);

    init_instructions();

    while (emu->eip < MEMORY_SIZE) {
        uint8_t code = get_code8(emu, 0);
        debugf("EIP = %X, Code = %02X\n", emu->eip, code);

        if (instructions[code] == NULL) {
            printf("\n\nNot Implemented: %x\n", code);
            break;
        }
        instructions[code](emu);

        // Exit if jump to 0x00
        if (emu->eip == 0x00) {
            debugf("\n\nend of program.\n\n");
            break;
        }
    }

    dump_registers(emu);

    int exit_status = (int) emu->registers[EAX];
    destroy_emu(emu);
    return exit_status;
}
