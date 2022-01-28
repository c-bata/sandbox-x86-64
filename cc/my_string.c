#include <stdlib.h>

char* my_strndup(const char* s, int len) {
    // Write my own strndup function to avoid
    // 'strndup.c no such file or directory' error.
    char* dup = malloc(sizeof(char)*(len+1));
    int i;
    for (i=0; i<len; i++) {
        dup[i] = s[i];
    }
    dup[len] = '\0';
    return dup;
}
