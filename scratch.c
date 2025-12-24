#include <stdio.h>

void printHash(char *text) {
    char *t = text;
    int result = 0;
    while(*t != '\0') {
        result = result ^ *t;
        t++;
    }

    printf("case %d: method = %s; break;\n", result, text);
}

int main() {
    printHash("GET");
    printHash("POST");
    printHash("DELETE");
    printHash("PUT");
    printHash("CONNECT");
    printHash("PATCH");
    printHash("TRACE");
    printHash("HEAD");
    printHash("OPTIONS");
}
