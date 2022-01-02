#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <ctype.h>
#include "9cc.h"

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

int main(int argc, char **argv) {
    CodeGenOption* options = calloc(1, sizeof(CodeGenOption));
    int i = 1;
    while (i < argc) {
        if (strcmp(argv[i], "--cpuemu") == 0) {
            options->cpu_emu = true;
            argc = opt_remove_at(argc, argv, i);
        } else {
            i++;
        }
    }

    if (argc != 2) {
        error("error: no input program.");
    }

    Token *tok = tokenize(argv[1]);
    Node *node = parse(tok);
    codegen(node, options);
    return 0;
}
