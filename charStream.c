#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "charStream.h"

struct CharStream {
    void *state;
    char (*next)(void *state);
    int (*eof)(void *state);
    void (*free)(void *state);
    char current;
};

char peek(CharStream *stream) {
    return stream->current;
}

char next(CharStream *stream) {
    char peeked = peek(stream);

    if (stream->eof(stream->state)) {
        if (peeked == '\0') {
            perror("Tried to get next while eof");
            exit(1);
        }

        stream->current = '\0';
    } else {
        stream->current = stream->next(stream->state);
    }

    return peeked;
}

int eof(CharStream *stream) {
    return stream->eof(stream->state) && stream->current == '\0';
}

void initializeStream(CharStream *stream) {
    stream->current = stream->next(stream->state);
}

void freeStream(CharStream *stream) {
    stream->free(stream->state);
    free(stream->state);
    free(stream);
}

// ------ STRING IMPLEMENTATION ------

typedef struct {
    char *data;
    int index;
} StringStreamState;

char stringStreamNext(void *vs) {
    StringStreamState *state = (StringStreamState*) vs;
    return state->data[state->index++];
}

int stringStreamEof(void *vs) {
    StringStreamState *state = (StringStreamState*) vs;
    return state->data[state->index] == '\0';
}

void stringStreamFree(void *vs) {
    StringStreamState *state = (StringStreamState*) vs;
    free(state->data);
}

CharStream *createStringStream(char *data) {
    StringStreamState *state = calloc(1, sizeof(StringStreamState));
    state->data = strdup(data);
    state->index = 0;

    CharStream *stream = calloc(1, sizeof(CharStream));
    stream->state = state;
    stream->next = &stringStreamNext;
    stream->eof = &stringStreamEof;
    stream->free = &stringStreamFree;
    initializeStream(stream);

    return stream;
}
