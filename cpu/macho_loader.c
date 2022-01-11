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
#include <assert.h>

typedef struct {
    uint64_t seg_text_vmaddr;
    uint64_t seg_text_vmsize;

    uint64_t sect_text_vmaddr;
    uint64_t sect_text_offset;
    size_t sect_text_size;

    // Always 4GB. the file specifies that the first 4GB of the process' address space
    // will be mapped as non-executable, non-writable, non-readable.
    uint64_t pagezero_vmaddr;
    uint64_t pagezero_vmsize;

    uint64_t lc_main_entryoff;	// file (__TEXT) offset of main()
    uint64_t lc_main_stacksize; // if not zero, initial stack size
} MachO;

void parse_macho64(void *head, MachO *macho) {
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
    for (i=0; i<=header->ncmds; i++) {
        struct load_command *cmd = (struct load_command *) ptr;
        if (cmd->cmd == LC_SEGMENT_64) {
            struct segment_command_64 *seg = (struct segment_command_64 *) ptr;
            if (strcmp(seg->segname, SEG_TEXT) == 0) {
                macho->seg_text_vmaddr = seg->vmaddr;
                macho->seg_text_vmsize = seg->vmsize;
                for (j=0; j<seg->nsects; j++) {
                    struct section_64 *sec = (struct section_64 *) (ptr + sizeof(struct segment_command_64));
                    if (strcmp(sec->sectname, SECT_TEXT) == 0) {
                        macho->sect_text_vmaddr = sec->addr;
                        macho->sect_text_offset = sec->offset;
                        macho->sect_text_size = sec->size;
                    }
                }
            } else if (strcmp(seg->segname, SEG_PAGEZERO) == 0) {
                macho->pagezero_vmaddr = seg->vmaddr;
                macho->pagezero_vmsize = seg->vmsize;
            }
        } else if (cmd->cmd == LC_MAIN) {
            struct entry_point_command *entrypoint = (struct entry_point_command *) ptr;
            macho->lc_main_entryoff = entrypoint->entryoff;
            macho->lc_main_stacksize = entrypoint->stacksize;
        }
        ptr += cmd->cmdsize;
    }
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

    MachO* macho = malloc(sizeof(MachO));
    head = mmap(NULL, sb.st_size, PROT_READ, MAP_SHARED, fd, 0);

    // TODO: Pass Emulator* object instead of MachO*.
    parse_macho64(head, macho);

    uint64_t rip = macho->seg_text_vmaddr + macho->lc_main_entryoff;
    assert(macho->sect_text_vmaddr == rip);
    uint64_t rsp = macho->pagezero_vmaddr + macho->pagezero_vmsize;

    Emulator* emu = create_emu(rip, rsp);
    vm_memcpy(emu->memory, macho->sect_text_vmaddr, head + macho->sect_text_offset, macho->sect_text_size);

    munmap(head, sb.st_size);
    free(macho);
    close(fd);
    return emu;
}
#endif
