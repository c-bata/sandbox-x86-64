#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <sys/ptrace.h>
#include <sys/wait.h>
#include <sys/user.h>
#include <string.h>

enum ndbcmd_t {
    ndbcmd_readregs,
    ndbcmd_readmem,
    ndbcmd_writemem,
    ndbcmd_quit,
};

int main(int argc, char *argv[], char *envp[]) {
    int status;
    int child_pid;
    struct user_regs_struct regs;

    child_pid = fork();
    if(child_pid == -1 ) {
        fprintf(stderr, "Error forking: %s\n", strerror(errno));
        exit(-1);
    }
    if (!child_pid) { /* tracee process */
        close(1);
        ptrace(PTRACE_TRACEME, 0, NULL, NULL);
        execve(argv[1], argv + 1, envp);
    }
    while (1) {
        waitpid(child_pid, &status, 0);
        if (WIFEXITED(status)) {
            // 子が正常に終了した
            break;
        } else if (WIFSIGNALED(status)) {
            // 子がシグナルによって終了した
            exit(1);
        } else if (WIFSTOPPED(status)) {
            // 子がシグナルによって停止した
            int signum = WSTOPSIG(status);
            if (signum == SIGTRAP) {
                // execveの実行などによるSIGTRAP
                // ...
            } else if (signum == SIGSEGV) {
                printf("Program received signal SIGSEGV.\n");
                ptrace(PTRACE_GETREGS, child_pid, 0, &regs);
                // RIPレジスタの表示
                fprintf(stderr, "RIP=%llx, RAX=%llx\n", regs.rip, regs.orig_rax);
                
                // ユーザーからコマンドを受け付けるループ (一旦省略)
            }
        }
    }
    return 0;
}