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
        if (current->kind == TK_IDENT) {
            char base[128] = "IDENT: \t";
            strncat(base, current->loc, current->len);
            printf("%s\n", base);
        } else if (current -> kind == TK_PUNCT) {
            char base[128] = "PUNCT: \t";
            strncat(base, current->loc, current->len);
            printf("%s\n", base);
        } else if (current -> kind == TK_KEYWORD) {
            char base[128] = "KEYWORD: \t";
            strncat(base, current->loc, current->len);
            printf("%s\n", base);
        } else if (current -> kind == TK_NUM) {
            char base[128] = "NUMBER: \t";
            strncat(base, current->loc, current->len);
            printf("%s", base);
            printf("\tval=%d\n", current->val);
        } else if (current -> kind == TK_STR) {
            char base[128] = "STRING: \t";
            strncat(base, current->loc, current->len);
            printf("%s", base);
            printf("\tstr=%s\n", current->str);
        } else if (current -> kind == TK_EOF) {
            printf("EOF\n");
        } else {
            printf("UNEXPECTED\n");
        }
    }
    return 0;
}
