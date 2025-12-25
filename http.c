#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "http.h"

typedef struct {
    char *chars;
    int length;  // Length excluding \0
} String;

int collectUntilCharBuffered(CharStream *stream, char *buffer, int bufferLength, char until) {
    int n = 0;
    while(!eof(stream) && peek(stream) != until && bufferLength > n + 1) {
        buffer[n++] = next(stream);
    }
    buffer[n] = '\0';
    
    return n;
}

String collectUntilChar(CharStream *stream, char until) {
    char *result = malloc(0);
    int n = 0;
    while(!eof(stream) && peek(stream) != until) {
        result = realloc(result, ++n * sizeof(char));
        result[n - 1] = next(stream);
    }
    
    result = realloc(result, (n + 1) * sizeof(char));
    result[n] = '\0';
    String s = { result, n };
    return s;
}

int stringHash(char *t) {
    int result = 0;
    while(*t != '\0') {
        result = result ^ *t;
        t++;
    }

    return result;
}

int parseRequestLine(HttpRequest *result, CharStream *stream) {
    char buffer[16];
    collectUntilCharBuffered(stream, buffer, 16, ' ');

    RequestMethod method;
    switch (stringHash(buffer)) { // this is stupid
        case 86: method = GET; break;
        case 24: method = POST; break;
        case 25: method = DELETE; break;
        case 81: method = PUT; break;
        case 94: method = CONNECT; break;
        case 78: method = PATCH; break;
        case 65: method = TRACE; break;
        case 8: method = HEAD; break;
        case 80: method = OPTIONS; break;
    }
    result->method = method;

    next(stream); // Skip ' '
    
    String targetString = collectUntilChar(stream, ' ');
    result->target = targetString.chars;

    next(stream); // Skip ' '

    String httpVersion = collectUntilChar(stream, '\r');

    if(strncmp(httpVersion.chars, "HTTP/", 5)) {
        errno = EPROTO;
        return 0;
    }

    result->httpVersion = strdup(httpVersion.chars + 5);  // Cutting HTTP/ from the start
    free(httpVersion.chars);

    next(stream); // Skip '\r
    next(stream); // Skip '\n'

    return 1;
}

int parseHeaders(HttpRequest *r, CharStream *stream) {
    RequestHeader *headers = malloc(0);
    int n = 0;

    //TODO a bit weak condition, heavily assuming '\r\n'
    while(!eof(stream) && peek(stream) != '\r') {
        // Skipping OWS
        while(!eof(stream) && peek(stream) == ' ') {
            next(stream);
        } 

        if(peek(stream) == '\r' || peek(stream) == '\n') {
            errno = EPROTO;
            return 0;
        }

        RequestHeader header;
        
        // Reading key
        header.key = malloc(0);
        int i = 0;
        while(!eof(stream) && peek(stream) != ':') {
            if(peek(stream) == ' ' || peek(stream) == '\n' || peek(stream) == '\r') {
                errno = EPROTO;  // Not allowing whitespaces before colon
                return 0;
            }
            header.key = realloc(header.key, ++i);
            header.key[i - 1] = next(stream);
        } 

        if (i == 0) {  // Key cannot be empty
            errno = EPROTO;
            return 0;
        }

        next(stream); // Skip ':'

        // Skipping OWS
        while(!eof(stream) && peek(stream) == ' ') {
            next(stream);
        } 

        // Reading value. Here, we read until line break and trim optional OWS
        header.value = malloc(0);
        i = 0;
        int lastWhitespaceStart = -1;
        while(!eof(stream) && peek(stream) != '\r' && peek(stream) != '\n') {
            header.value = realloc(header.value, ++i);
            char c = next(stream);
            header.value[i - 1] = c;

            if (c != ' ') {
                lastWhitespaceStart = -1;
            } else if (i > 1 && header.value[i - 2] != ' ') {
                lastWhitespaceStart = i - 2;
            }
        } 

        if (i == 0) {  // Value cannot be empty
            errno = EPROTO;
            return 0;
        }

        char *withoutOWS = malloc(lastWhitespaceStart + 2); // +1 for actual size, +1 for \0
        strncpy(withoutOWS, header.value, lastWhitespaceStart + 1);
        withoutOWS[lastWhitespaceStart + 1] = '\0';
        free(header.value);
        header.value = withoutOWS;

        next(stream); // Skip '\r
        next(stream); // Skip '\n'
        
        headers = realloc(headers, ++n * sizeof(RequestHeader));
        headers[n - 1] = header;
    }
    
    next(stream); // Skip '\r
    next(stream); // Skip '\n'

    r->headers = headers;
    r->headersSize = n;
    return 1;
}

HttpRequest *parseHttpRequest(CharStream *stream) {
    HttpRequest *result = calloc(1, sizeof(HttpRequest));

    int success = parseRequestLine(result, stream);
    if (!success) return NULL;

    success = parseHeaders(result, stream);
    if (!success) return NULL;

    return result;
}

char *getHeaderValue(HttpRequest *r, char *key) {
    RequestHeader h;
    for (int i = 0; i < r->headersSize; i++) {
        h = r->headers[i];
        if (!strcmp(h.key, key)) {
            return h.value;
        }
    }

    return NULL;
}

void freeHttpRequest(HttpRequest *r) {
    free(r->target);
    free(r->httpVersion);
    free(r->body);

    for (int i = 0; i < r->headersSize; i++) {
        RequestHeader h = r->headers[i];
        free(h.key);
        free(h.value);
    }
    free(r->headers);

    free(r);
}
