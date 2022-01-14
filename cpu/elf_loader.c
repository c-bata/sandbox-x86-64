#include <stdlib.h>
#include <unistd.h>
#include "emulator.h"

#ifdef __linux

#include <elf.h>
#include <fcntl.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include "elf_loader.h"
#include "emulator_function.h"

#define IS_ELF64(ehdr) \
  ((ehdr).e_ident[EI_MAG0] == ELFMAG0 && \
   (ehdr).e_ident[EI_MAG1] == ELFMAG1 && \
   (ehdr).e_ident[EI_MAG2] == ELFMAG2 && \
   (ehdr).e_ident[EI_MAG3] == ELFMAG3 && \
   (ehdr).e_ident[EI_CLASS] == ELFCLASS64)

void parse_elf64(void *head, Emulator* emu) {
    int i;
    Elf64_Ehdr *ehdr;
    ehdr = (Elf64_Ehdr *)head;
    if (!IS_ELF64(*ehdr)) {
        fprintf(stderr, "This is not ELF64 file.\n");
        exit(1);
    }
    if (ehdr->e_machine != EM_X86_64) {
        fprintf(stderr, "This emulator only supports executable for x86-64.\n");
        exit(1);
    }

    // TODO: Get vmaddr of main from symbol table.
    // readelf -s ./tmp.exe
    //    53: 0000000000004018     0 NOTYPE  GLOBAL DEFAULT   23 _end
    //    54: 0000000000001040    47 FUNC    GLOBAL DEFAULT   13 _start
    //    55: 0000000000004010     0 NOTYPE  GLOBAL DEFAULT   23 __bss_start
    //    56: 0000000000001129     0 NOTYPE  GLOBAL DEFAULT   13 main
    // emu->rip = ehdr->e_entry;
    emu->rip = 0x1129;  // main

    Elf64_Phdr *phdr;
    for (i = 0; i < ehdr->e_phnum; i++) {
        phdr = (Elf64_Phdr *) (head + ehdr->e_phoff + ehdr->e_phentsize * i);
        if (phdr->p_type == PT_LOAD) {
            vm_memcpy(emu->memory, phdr->p_vaddr,
                      head+phdr->p_offset, phdr->p_filesz);
        }
    }
    // It is not required to clear .bss fields by zero because memories are initialized with 0.
}

Emulator* load_elf64(char* filepath) {
    int fd;
    char *filename, *head;
    filename = filepath;
    fd = open(filename, O_RDONLY);
    if (fd == -1) {
        fprintf(stderr, "'%s' could not be opened.\n", filename);
        return NULL;
    }
    struct stat sb;
    fstat(fd, &sb);

    head = mmap(NULL, sb.st_size, PROT_READ, MAP_SHARED, fd, 0);
    Emulator* emu = create_emu(0x0, 0x0);  // Set dummy RIP and RSP.
    parse_elf64(head, emu);

    munmap(head, sb.st_size);
    close(fd);

    push64(emu, 0x00); // Push return address
    return emu;
}

#else

Emulator* load_elf64(char* filepath) {
    fprintf(stderr, "ELF 64 binary is not supported on macOS\n");
    exit(EXIT_FAILURE);
}

#endif
