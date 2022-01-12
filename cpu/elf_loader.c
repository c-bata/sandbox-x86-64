#include <stdlib.h>
#include "elf_loader.h"

#ifdef __linux

Emulator* load_elf64(char* filepath) {
    fprintf(stderr, "Implemented me\n");
    exit(EXIT_FAILURE);
}

#else

Emulator* load_elf64(char* filepath) {
    fprintf(stderr, "ELF 64 binary is not supported on macOS\n");
    exit(EXIT_FAILURE);
}

#endif
