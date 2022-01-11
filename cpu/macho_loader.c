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

typedef struct {
    uint64_t vm_addr;
    uint64_t machine_code_offset;
    size_t machine_code_size;
} MachO;

static void *load_bytes(FILE *obj_file, off_t offset, size_t size) {
    void *buf = malloc(size);
    fseek(obj_file, offset, SEEK_SET);
    fread(buf, size, 1, obj_file);
    return buf;
}

void load_macho(void *head, MachO *macho) {
    int i, j;
    struct mach_header_64 *header = (struct mach_header_64 *) head;
    int is_64 = header->magic == MH_MAGIC_64 || header->magic == MH_CIGAM_64;
    int fat = header->magic == FAT_MAGIC || header->magic == FAT_CIGAM;
    if (!is_64 || fat || header->cputype != CPU_TYPE_X86_64) {
        fprintf(stderr, "unsupported file: is_64=%d, fat=%d, cputype=%d",
                is_64, fat, header->cputype);
        exit(1);
    }

    void *ptr = head + sizeof(struct mach_header_64);
    for (i=0; i<=header->ncmds; i++) {
        struct load_command *cmd = (struct load_command *) ptr;
        if (cmd->cmd == LC_SEGMENT_64) {
            struct segment_command_64 *seg = (struct segment_command_64 *) ptr;
            if (strcmp(seg->segname, "__TEXT") == 0) {
                for (j=0; j<seg->nsects; j++) {
                    struct section_64 *sec = (struct section_64 *) (ptr + sizeof(struct segment_command_64));
                    if (strcmp(sec->sectname, "__text") == 0) {
                        macho->vm_addr = sec->addr;
                        macho->machine_code_offset = sec->offset;
                        macho->machine_code_size = sec->size;
                    }
                }
            }
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
    load_macho(head, macho);

    Emulator* emu = create_emu(macho->vm_addr, macho->vm_addr);
    vm_memcpy(emu->memory, macho->vm_addr, head+macho->machine_code_offset, macho->machine_code_size);

    munmap(head, sb.st_size);
    free(macho);
    close(fd);
    return emu;
}
#endif
