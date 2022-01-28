#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include "emulator.h"
#include "emulator_function.h"

#ifdef __linux

Emulator* load_macho64(char* filepath) {
    fprintf(stderr, "Mach-O 64 binary is not supported on Linux\n");
    exit(EXIT_FAILURE);
}

#else

#include <mach-o/loader.h>
#include <mach-o/fat.h>

void parse_macho64(void *head, Emulator *emu) {
    int i, j;
    struct mach_header_64 *header = (struct mach_header_64 *) head;
    int is_executable = header->filetype == MH_EXECUTE;
    int is_64 = header->magic == MH_MAGIC_64 || header->magic == MH_CIGAM_64;
    int fat = header->magic == FAT_MAGIC || header->magic == FAT_CIGAM;
    if (!is_executable || !is_64 || fat || header->cputype != CPU_TYPE_X86_64) {
        fprintf(stderr, "unsupported file: is_executable=%d, is_64=%d, fat=%d, cputype=%d",
                is_executable, is_64, fat, header->cputype);
        exit(1);
    }

    void *ptr = head + sizeof(struct mach_header_64);
    uint64_t seg_text_vmaddr = 0, lc_main_entryoff = 0;
    uint64_t pagezero_vmaddr = 0, pagezero_vmsize = 0;
    for (i=0; i<=header->ncmds; i++) {
        struct load_command *cmd = (struct load_command *) ptr;
        if (cmd->cmd == LC_SEGMENT_64) {
            struct segment_command_64 *seg = (struct segment_command_64 *) ptr;
            if (strcmp(seg->segname, SEG_TEXT) == 0) {
                seg_text_vmaddr = seg->vmaddr;
                vm_memcpy(emu->memory, seg->vmaddr, head + seg->fileoff, seg->vmsize);
            } else if (strcmp(seg->segname, SEG_DATA) == 0) {
                vm_memcpy(emu->memory, seg->vmaddr, head + seg->fileoff, seg->vmsize);
            } else if (strcmp(seg->segname, SEG_PAGEZERO) == 0) {
                // Always 4GB. the file specifies that the first 4GB of the process' address space
                // will be mapped as non-executable, non-writable, non-readable.
                pagezero_vmaddr = seg->vmaddr;
                pagezero_vmsize = seg->vmsize;
            }
        } else if (cmd->cmd == LC_MAIN) {
            struct entry_point_command *entrypoint = (struct entry_point_command *) ptr;
            lc_main_entryoff = entrypoint->entryoff;
            // lc_main_stacksize = entrypoint->stacksize;
        }
        ptr += cmd->cmdsize;
    }
    emu->rip = seg_text_vmaddr + lc_main_entryoff;
    emu->registers[RSP] = pagezero_vmaddr + pagezero_vmsize;
    push64(emu, 0x0); // Push return address
}

Emulator* load_macho64(char* filepath) {
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
    Emulator* emu = create_emu(0x0, 0x0);
    parse_macho64(head, emu);
    munmap(head, sb.st_size);
    close(fd);

    return emu;
}
#endif
