#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/ptrace.h>
#include <sys/wait.h>
#include <sys/user.h>
#include <string.h>
#include <elf.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <stdbool.h>

#define IS_ELF64(ehdr) \
  ((ehdr).e_ident[EI_MAG0] == ELFMAG0 && \
   (ehdr).e_ident[EI_MAG1] == ELFMAG1 && \
   (ehdr).e_ident[EI_MAG2] == ELFMAG2 && \
   (ehdr).e_ident[EI_MAG3] == ELFMAG3 && \
   (ehdr).e_ident[EI_CLASS] == ELFCLASS64)

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

uint64_t parse_elf64(char* filepath) {
    int fd;
    char *filename, *head;
    filename = filepath;
    fd = open(filename, O_RDONLY);
    if (fd == -1) {
        fprintf(stderr, "'%s' could not be opened.\n", filename);
        return -1;
    }
    struct stat sb;
    fstat(fd, &sb);

    head = mmap(NULL, sb.st_size, PROT_READ, MAP_SHARED, fd, 0);
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
    uint64_t main_vmaddr = find_main_sym_addr(head);

    munmap(head, sb.st_size);
    close(fd);
    return main_vmaddr;
}

uint8_t replace_instruction_code(int pid, uint64_t addr, uint8_t val) {
    long original;
    original = ptrace(PTRACE_PEEKTEXT, pid, addr, NULL);
    fprintf(stderr, "PEEKTEXT: 0x%lx.\n", original);
    ptrace(PTRACE_POKETEXT, pid, addr, ((original & 0xFFFFFFFFFFFFFF00) | val));
    fprintf(stderr, "Breakpoint at 0x%lx.\n", addr);
    return original & 0xFF;
}

int main(int argc, char *argv[], char *envp[]) {
    int pid, waitstatus;
    struct user_regs_struct regs;

    pid = fork();
    if( pid == -1 ) {
        fprintf(stderr, "Error forking: %s\n", strerror(errno));
        exit(-1);
    }
    if (!pid) { /* tracee process */
        close(1);
        ptrace(PTRACE_TRACEME, 0, NULL, NULL);
        execve(argv[1], argv + 1, envp);
    }

    uint64_t main_vmaddr = parse_elf64(argv[1]);
    printf("main_vmaddr 0x%lx\n", main_vmaddr);
    waitpid(pid, &waitstatus, 0);

    /*
    uint8_t orig_code = replace_instruction_code(pid, main_vmaddr, 0xCC);
    if (ptrace(PTRACE_CONT, pid, NULL, NULL)) {
        perror("PTRACE_CONT");
        exit(1);
    }
    waitpid(pid, &waitstatus, 0);
    uint8_t code_0xCC = replace_instruction_code(pid, main_vmaddr, orig_code);
    fprintf(stderr, "Must be 0xCC: actual=0x%x\n", code_0xCC);
     */

    bool entrypoint_found = false;
    while (WIFSTOPPED(waitstatus)) { /* tracer loop */
        if (ptrace(PTRACE_GETREGS, pid, NULL, &regs)) {
            perror("PTRACE_GETREGS");
            exit(1);
        }

        if (regs.rax == 0x2a) {
            long int instr = ptrace(PTRACE_PEEKDATA, pid, regs.rip, 0); // read 4 bytes
            fprintf(stderr, "RIP=%llx, RAX=%llx, ORIG_RAX=%llx, instr=0x%lx\n",
                    regs.rip, regs.rax, regs.orig_rax, instr);
        }

        if (ptrace(PTRACE_SINGLESTEP, pid, NULL, NULL)) {
            perror("PTRACE_SINGLESTEP");
            exit(1);
        }
        waitpid(pid, &waitstatus, 0);
    }
    fprintf(stderr, "Finished\n");
    exit(0);
}
