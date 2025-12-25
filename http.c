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

    skip(stream); // Skip ' '
    
    String targetString = collectUntilChar(stream, ' ');
    result->target = targetString.chars;

    skip(stream); // Skip ' '

    String httpVersion = collectUntilChar(stream, '\r');

    if(strncmp(httpVersion.chars, "HTTP/", 5)) {
        errno = EPROTO;
        return 0;
    }

    result->httpVersion = strdup(httpVersion.chars + 5);  // Cutting HTTP/ from the start
    free(httpVersion.chars);

    skip(stream); // Skip '\r
    skip(stream); // Skip '\n'

    return 1;
}

int validateHeader(RequestHeader h) {
    char c;
    int i = 0;
    while ((c = h.key[i++]) != '\0') {
        if (!(
            (c >= '!' && c <= '.' && c != '"' && c != '(' && c != ')' && c != ',' || c >= '^' && c <= '`' || c == '|' || c == '~')   // Special chars
            || (c >= '0' && c <= '9') || (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z')
        )) {
            errno = EPROTO;
            return 0;
        }
    }

    return 1;
}

int parseHeaders(HttpRequest *r, CharStream *stream) {
    RequestHeader *headers = malloc(0);
    int n = 0;

    //TODO a bit weak condition, heavily assuming '\r\n'
    while(!eof(stream) && peek(stream) != '\r') {
        // Skipping OWS
        while(!eof(stream) && peek(stream) == ' ') {
            skip(stream);
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

        if(!validateHeader(header)) return 0;

        skip(stream); // Skip ':'

        // Skipping OWS
        while(!eof(stream) && peek(stream) == ' ') {
            skip(stream);
        } 

        // Reading value. Here, we read until line break and trim optional OWS
        header.value = malloc(0);
        int valueN = 0;
        int lastWhitespaceStart = -1;
        while(!eof(stream) && peek(stream) != '\r' && peek(stream) != '\n') {
            header.value = realloc(header.value, ++valueN);
            char c = next(stream);
            header.value[valueN - 1] = c;

            if (c != ' ') {
                lastWhitespaceStart = -1;
            } else if (valueN > 1 && header.value[valueN - 2] != ' ') {
                lastWhitespaceStart = valueN - 2;
            }
        } 

        if (valueN == 0) {  // Value cannot be empty
            errno = EPROTO;
            return 0;
        }

        if (lastWhitespaceStart > 0) {
            char *withoutOWS = malloc(lastWhitespaceStart + 2); // +1 for actual size, +1 for \0
            strncpy(withoutOWS, header.value, lastWhitespaceStart + 1);
            withoutOWS[lastWhitespaceStart + 1] = '\0';
            free(header.value);
            header.value = withoutOWS;
        }

        skip(stream); // Skip '\r
        skip(stream); // Skip '\n'
        
        // If there is duplicate key, merge the value, add the key-value pair otherwise
        int foundDuplicate = -1;
        for (int i = 0; i < n; i++) {
            if (!strcmp(headers[i].key, header.key)) {
                foundDuplicate = i;
                break;
            }
        }

        if (foundDuplicate < 0) {
            headers = realloc(headers, ++n * sizeof(RequestHeader));
            headers[n - 1] = header;
        } else {
            RequestHeader found = headers[foundDuplicate];
            int foundLen = strlen(found.value);
            char *catValue = malloc(foundLen + 1 + valueN + 1);
            strncpy(catValue, found.value, foundLen);
            catValue[foundLen] = ',';
            strncpy(catValue + foundLen + 1, header.value, valueN);

            free(found.value);
            free(header.value);
            free(header.key);
            headers[foundDuplicate].value = catValue;
        }
    }
    
    skip(stream); // Skip '\r
    skip(stream); // Skip '\n'

    r->headers = headers;
    r->headersSize = n;
    return 1;
}

int parseBody(HttpRequest *r, CharStream *stream) {
    char *lenstr = getHeaderValue(r, "Content-Length");
    if (lenstr == NULL) return 1;  // No body
    int len = atol(lenstr);

    r->bodySize = len;
    r->body = malloc(len + 1);

    for (int i = 0; i < len; i++) {
        if (eof(stream)) {
            errno = EPROTO;
            return 0;
        }

        r->body[i] = skip(stream);
    }
    r->body[len] = '\0';

    return 1;
}

HttpRequest *parseHttpRequest(CharStream *stream) {
    HttpRequest *result = calloc(1, sizeof(HttpRequest));

    int success = parseRequestLine(result, stream);
    if (!success) return NULL;

    success = parseHeaders(result, stream);
    if (!success) return NULL;

    success = parseBody(result, stream);
    if (!success) return NULL;

    return result;
}

char *getHeaderValue(HttpRequest *r, char *key) {
    RequestHeader h;
    for (int i = 0; i < r->headersSize; i++) {
        h = r->headers[i];
        if (!strcasecmp(h.key, key)) {
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
