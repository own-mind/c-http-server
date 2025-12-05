#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
    char **lines;
    int size;
} LineArray;

LineArray getLines(FILE *file) {
    char line[256];
    char **result = malloc(0);
    int n = 0;
    
    size_t lineSize;
    char *resultLine;
    while (fgets(line, sizeof(line), file) != NULL) {
        result = realloc(result, ++n * sizeof(char*));

        lineSize = strlen(line);
        resultLine = malloc((lineSize + 1) * sizeof(char));
        strncpy(resultLine, line, lineSize);

        result[n - 1] = resultLine;
    }
    
    LineArray arr = { result, n } ;
    return arr;
}

int main() {
    FILE *file = fopen("message.txt", "r");

    LineArray arr = getLines(file);
    for (int i = 0; i < arr.size; i++) {
        printf("read: %s\n", arr.lines[i]);
        free(arr.lines[i]);
    }

    free(arr.lines);
    
    fclose(file);
}
