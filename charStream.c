#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdbool.h>
#include "charStream.h"

struct CharStream {
    void *state;
    char (*next)(void *state);
    int (*eof)(void *state);
    void (*free)(void *state);

    char current;
    bool peeked;
};

char peek(CharStream *stream) {
    if (!stream->peeked) {
        stream->current = stream->next(stream->state);
        stream->peeked = true;
    }
    return stream->current;
}

/**
 * Skips current char without peeking into the next (unlike next function)
 */
char skip(CharStream *stream) {
    if (!stream->peeked) {
        return stream->next(stream->state);
    }

    stream->peeked = false;
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
    return stream->eof(stream->state) && (!stream->peeked || stream->current == '\0');
}

void initializeStream(CharStream *stream) {
    stream->peeked = false;
}

void freeStream(CharStream *stream) {
    stream->free(stream->state);
    free(stream->state);
    free(stream);
}

// ------ SOCKET IMPLEMENTATION ------

typedef struct {
    int clientSockfd;
    ssize_t lastResvSize;   // If equals 0, EOF
} SocketStreamState;

char socketStreamNext(void *vs) {
    SocketStreamState *state = (SocketStreamState*) vs;

    char c;   //TODO make it a buffered state, not just reading one by one
    state->lastResvSize = recv(state->clientSockfd, &c, 1, 0);
    
    if (state->lastResvSize > 0) {
        return c;
    } else {
        return 0;
    }
}

int socketStreamEof(void *vs) {
    SocketStreamState *state = (SocketStreamState*) vs;
    return state->lastResvSize <= 0;
}

void socketStreamFree(void *vs) {
}

CharStream *createSocketStream(int clientSockfd) {
    SocketStreamState *state = calloc(1, sizeof(SocketStreamState));
    state->lastResvSize = 1;
    state->clientSockfd = clientSockfd;

    CharStream *stream = calloc(1, sizeof(CharStream));
    stream->state = state;
    stream->next = &socketStreamNext;
    stream->eof = &socketStreamEof;
    stream->free = &socketStreamFree;
    initializeStream(stream);

    return stream;
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
