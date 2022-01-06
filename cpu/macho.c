#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <mach-o/loader.h>
#include <mach-o/fat.h>

#include "emulator.h"
#include "emulator_function.h"

#define MEMORY_SIZE (1024 * 1024)

typedef struct {
    uint64_t vm_addr;
    uint64_t machine_code_offset;
    size_t machine_code_size;
} MachO;

static void dump_bytes(uint8_t* bytes, size_t size) {
    int i;
    for (i=0; i<size; i++) {
        if (i == 0)
            printf("\t");
        else if (i%10 == 0)
            printf("\n\t");
        printf("%02X", bytes[i]);
    }
    printf("\n");
}

static void *load_bytes(FILE *obj_file, off_t offset, size_t size) {
    void *buf = malloc(size);
    fseek(obj_file, offset, SEEK_SET);
    fread(buf, size, 1, obj_file);
    return buf;
}

static void load_section_64(FILE *obj_file, off_t offset, MachO *macho) {
    struct section_64 *sec = load_bytes(obj_file, offset, sizeof(struct section_64));
    if (strcmp(sec->sectname, "__text") == 0) {
        macho->vm_addr = sec->addr;
        macho->machine_code_offset = sec->offset;
        macho->machine_code_size = sec->size;
    }
    free(sec);
}

static void load_segment_command_64(FILE *obj_file, off_t offset, MachO *macho) {
    struct segment_command_64 *seg = load_bytes(obj_file, offset, sizeof(struct segment_command_64));
    if (strcmp(seg->segname, "__TEXT") == 0) {
        int i;
        offset += sizeof(struct segment_command_64);
        for (i=0; i<seg->nsects; i++) {
            load_section_64(obj_file, offset, macho);
            offset += sizeof(struct section_64);
        }
    }
    free(seg);
}

void load_macho(FILE *obj_file, MachO *macho) {
    struct mach_header_64 *header = malloc(sizeof(struct mach_header_64));
    fread(header, sizeof(struct mach_header_64), 1, obj_file);

    int is_64 = header->magic == MH_MAGIC_64 || header->magic == MH_CIGAM_64;
    if (!is_64) {
        fprintf(stderr, "not implemented");
        exit(1);
    }
    int fat = header->magic == FAT_MAGIC || header->magic == FAT_CIGAM;
    if (fat) {
        fprintf(stderr, "fat header: not implemented");
        exit(1);
    }
    if (header->cputype != CPU_TYPE_X86_64) {
        fprintf(stderr, "cputype: not supported");
        exit(1);
    }
    uint32_t n_cmds = header->ncmds;
    free(header);

    struct load_command *cmd = malloc(sizeof(struct load_command));
    int i, offset = sizeof(struct mach_header_64);
    for (i=0; i<=n_cmds; i++) {
        fread(cmd, sizeof(*cmd), 1, obj_file);

        if (cmd->cmd == LC_SEGMENT_64) {
            load_segment_command_64(obj_file, offset, macho);
        }
        offset += cmd->cmdsize;
        fseek(obj_file, offset, SEEK_SET);
    }
    free(cmd);
}

Emulator* init_emu_from_macho(char* filepath) {
    FILE *obj_file = fopen(filepath, "rb");
    if (obj_file == NULL) {
        printf("'%s' could not be opened.\n", filepath);
        return NULL;
    }

    MachO* macho = malloc(sizeof(MachO));
    load_macho(obj_file, macho);
    Emulator* emu = create_emu(MEMORY_SIZE, macho->vm_addr, macho->vm_addr);

    fread(emu->memory + macho->vm_addr, 1, macho->machine_code_size, obj_file);

    fclose(obj_file);

    // DEBUG:
    dump_bytes(emu->memory, macho->machine_code_size);
    return emu;
}
