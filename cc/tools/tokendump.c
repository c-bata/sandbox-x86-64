#include <string.h>
#include <stdio.h>
#include "../9cc.h"

int main(int argc, char **argv) {
    if (argc != 2) {
        error("引数の個数が正しくありません");
        return 1;
    }

    int i = 0;
    Token *tok = tokenize(argv[1]);
    for (Token *current = tok; current; current=current->next) {
        printf("%02d. ", ++i);
        if (current->kind == TK_PUNCT) {
            char base[128] = "IDENT: \t";
            strncat(base, current->loc, current->len);
            printf("%s\n", base);
        // } else if (current -> kind == TK_RESERVED) {
        //     char base[128] = "RESERVED: \t";
        //     strncat(base, current->loc, current->len);
        //     printf("%s\n", base);
        } else if (current -> kind == TK_NUM) {
            char base[128] = "NUMBER: \t";
            strncat(base, current->loc, current->len);
            printf("%s\n", base);
        } else if (current -> kind == TK_EOF) {
            printf("EOF\n");
        } else {
            printf("UNEXPECTED\n");
        }
    }
    return 0;
}
