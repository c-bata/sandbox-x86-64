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

// CPU emulator currently cannot run _start routine (C Startup Script).
// So here we set RIP to the address of main routine instead of `ehdr->e_entry`.
uint64_t find_main_sym_addr(void *head) {
    int i, j;
    Elf64_Ehdr *ehdr = head;
    Elf64_Shdr *shstr = head + ehdr->e_shoff + ehdr->e_shentsize * ehdr->e_shstrndx;
    Elf64_Shdr *str;
    for (i = 0; i < ehdr->e_shnum; i++) {
        // Find .strtab section
        str = head + ehdr->e_shoff + ehdr->e_shentsize * i;
        char* segname = head + shstr->sh_offset + str->sh_name;
        if (!strcmp(segname, ".strtab"))
            break;
    }
    for (i = 0; i < ehdr->e_shnum; i++) {
        // Find main symbol
        Elf64_Shdr* sym = (head + ehdr->e_shoff + ehdr->e_shentsize * i);
        if (sym->sh_type != SHT_SYMTAB)
            continue;
        for (j=0; j<sym->sh_size / sym->sh_entsize; j++) {
            Elf64_Sym* symp = head + sym->sh_offset + sym->sh_entsize * j;
            char* symname = head + str->sh_offset + symp->st_name;
            if (strcmp(symname, "main") != 0)
                continue;
            return symp->st_value;
        }
    }
    fprintf(stderr, "ELF 64 Loader: main symbol is not found.");
    exit(EXIT_FAILURE);
}

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

    // It is not required to clear .bss fields by zero because memories are initialized with 0.
    Elf64_Phdr *phdr;
    for (i = 0; i < ehdr->e_phnum; i++) {
        phdr = (Elf64_Phdr *) (head + ehdr->e_phoff + ehdr->e_phentsize * i);
        if (phdr->p_type == PT_LOAD) {
            vm_memcpy(emu->memory, phdr->p_vaddr,
                      head+phdr->p_offset, phdr->p_filesz);
        }
    }

    emu->rip = find_main_sym_addr(head);
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

    // Set dummy RIP address which is overwritten in parse_elf64().
    // Here we set RSP as 0x8000000 since I found following interesting article
    // that says the first 128MB(0x80000000) is for stack.
    // https://eli.thegreenplace.net/2011/01/27/how-debuggers-work-part-2-breakpoints
    Emulator* emu = create_emu(0x0, 0x8000000);
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
