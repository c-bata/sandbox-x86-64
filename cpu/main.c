#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdarg.h>
#include <stdbool.h>

#include "emulator_function.h"
#include "elf_loader.h"
#include "macho_loader.h"
#include "instruction.h"

bool quiet = false;

enum formats {
    BIN,      // Flat raw binary [default]
    MACHO64,  // Mach-O x86-64 (Mach, including macOS variants)
    ELF64,    // ELF64(x86-64) (Linux, most Unix variants)
};
int format = BIN;

char* registers_name[] = {
        "RAX", "RCX", "RDX", "RBX", "RSP", "RBP", "RSI", "RDI",
        "R8", "R9", "R10", "R11", "R12", "R13", "R14", "R15"
};

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

static void dump_registers(Emulator* emu) {
    int i;
    for (i = 0; i < REGISTERS_COUNT; i++) {
        debugf("%s = %08x\n", registers_name[i], emu->registers[i]);
    }
    debugf("RIP = %08x\n", emu->rip);
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
    Emulator* emu;
    int i = 1;
    while (i < argc) {
        if (strcmp(argv[i], "-q") == 0 || strcmp(argv[i], "--quiet") == 0) {
            quiet = true;
            argc = opt_remove_at(argc, argv, i);
        } else if (strcmp(argv[i], "-f") == 0 || strcmp(argv[i], "--format") == 0) {
            argc = opt_remove_at(argc, argv, i);

            if (i >= argc)
                errorf("invalid --format option [bin, elf64, macho64]");

            if (strcmp(argv[i], "bin") == 0)
                format = BIN;
            else if (strcmp(argv[i], "elf64") == 0)
                format = ELF64;
            else if (strcmp(argv[i], "macho64") == 0)
                format = MACHO64;
            else
                errorf("invalid --format option [bin, elf64, macho64]");
            argc = opt_remove_at(argc, argv, i);
        } else {
            i++;
        }
    }

    if (format == MACHO64) {
        emu = load_macho64(argv[1]);
    } else if (format == ELF64) {
        emu = load_elf64(argv[1]);
    } else if (format == BIN) {
        emu = create_emu(0x7c00, 0x7c00);

        FILE* binary = fopen(argv[1], "rb");
        if (binary == NULL) {
            errorf("cannot open file '%s'\n", argv[1]);
        }
        // read machine program (max 512 bytes)
        vm_fread(emu->memory, 0x7c00, binary, 0x200);
        fclose(binary);
    } else {
        errorf("unsupported format: %d", format);
    }

    init_instructions();

    while (1) {
        uint8_t code = get_code8(emu, 0);
        debugf("RIP = %llx, Code = %02X\n", emu->rip, code);

        if (instructions[code] == NULL) {
            printf("\n\nNot Implemented: %x\n", code);
            break;
        }
        instructions[code](emu);

        // Exit if jump to 0x00
        if (emu->rip == 0x00) {
            debugf("\n\nend of program.\n\n");
            break;
        }
    }

    dump_registers(emu);

    int exit_status = (int) emu->registers[RAX];
    destroy_emu(emu);
    return exit_status;
}
