#include <stdio.h>
#include <string.h>
#include <stdlib.h>
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

static void usage() {
    fprintf(stderr, "9cc [ -o <path> ] <file>\n");
    exit(1);
}

int main(int argc, char **argv) {
    int i = 1;
    char *opt_o = NULL;
    while (i < argc) {
        if (strcmp(argv[i], "-o") == 0) {
            argc = opt_remove_at(argc, argv, i);
            opt_o = argv[i];
            argc = opt_remove_at(argc, argv, i);
        } else if (strcmp(argv[i], "--help") == 0 || strcmp(argv[i], "-h") == 0) {
            usage();
        } else {
            i++;
        }
    }

    if (argc != 2) {
        error("error: no input program.");
    }

    Token *tok = tokenize_file(argv[1]);
    Obj *prog = parse(tok);

    FILE* fout = stdout;
    if (opt_o != NULL)
        fout = fopen(opt_o, "w");
    if (fout == NULL)
        error("error: failed to open %s.", opt_o);

    codegen(prog, fout);

    if (opt_o == NULL)
        fclose(fout);
    return 0;
}
