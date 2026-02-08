#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "http.h"
#include "charStream.h"

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
    char *result = NULL;
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
        free(httpVersion.chars);
        errno = BAD_REQUEST;
        return 0;
    }

    result->httpVersion = strdup(httpVersion.chars + 5);  // Cutting HTTP/ from the start
    free(httpVersion.chars);

    if (strcmp(result->httpVersion, "1.1")) {   // Only 1.1 supported
        errno = HTTP_VERSION_NOT_SUPPORTED;
        return 0;
    }

    skip(stream); // Skip '\r
    skip(stream); // Skip '\n'

    return 1;
}

int validateHeader(Header h) {
    char c;
    int i = 0;
    while ((c = h.key[i++]) != '\0') {
        if (!(
            ((c >= '!' && c <= '.' && c != '"' && c != '(' && c != ')' && c != ',') || (c >= '^' && c <= '`') || c == '|' || c == '~')   // Special chars
            || (c >= '0' && c <= '9') || (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z')
        )) {
            errno = BAD_REQUEST;
            return 0;
        }
    }

    return 1;
}

int parseHeaders(HttpRequest *r, CharStream *stream) {
    Header *headers = NULL;
    int n = 0;

    //TODO a bit weak condition, heavily assuming '\r\n'
    while(!eof(stream) && peek(stream) != '\r') {
// Skipping OWS
        while(!eof(stream) && peek(stream) == ' ') {
            skip(stream);
        } 

        if(peek(stream) == '\r' || peek(stream) == '\n') {
            errno = BAD_REQUEST;
            return 0;
        }

        Header header;
        
        // Reading key
        header.key = NULL;
        int i = 0;
        while(!eof(stream) && peek(stream) != ':') {
            if(peek(stream) == ' ' || peek(stream) == '\n' || peek(stream) == '\r') {
                free(header.key);
                errno = BAD_REQUEST;  // Not allowing whitespaces before colon
                return 0;
            }
            header.key = realloc(header.key, ++i);
            header.key[i - 1] = next(stream);
        } 

        if (i == 0) {  // Key cannot be empty
            errno = BAD_REQUEST;
            return 0;
        }

        // Adding terminator
        header.key = realloc(header.key, ++i);
        header.key[i - 1] = '\0';

        if(!validateHeader(header)) { 
            free(header.key);
            return 0; 
        }


        skip(stream); // Skip ':'

        // Skipping OWS
        while(!eof(stream) && peek(stream) == ' ') {
            skip(stream);
        } 

        // Reading value. Here, we read until line break and trim OWS
        header.value = NULL;
        int valueN = 0;
        int lastWhitespaceStart = -1;
        while(!eof(stream) && peek(stream) != '\r' && peek(stream) != '\n') {
            char c = next(stream);
            header.value = realloc(header.value, ++valueN);
            header.value[valueN - 1] = c;

            if (c != ' ') {
                lastWhitespaceStart = -1;
            } else if (valueN > 1 && header.value[valueN - 2] != ' ') {
                lastWhitespaceStart = valueN - 2;
            }
        } 

        if (valueN == 0) {  // Value cannot be empty
            free(header.value);
            errno = BAD_REQUEST;
            return 0;
        }

        // Adding terminator
        header.value = realloc(header.value, ++valueN);
        header.value[valueN - 1] = '\0';

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
            headers = realloc(headers, ++n * sizeof(Header));
            headers[n - 1] = header;
        } else {
            Header found = headers[foundDuplicate];
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

    Headers hs = { headers, n };
    r->headers = hs;
    return 1;
}

int parseBody(HttpRequest *r, CharStream *stream) {
    char *lenstr = getHeaderValue(r->headers, "Content-Length");
    if (lenstr == NULL) {
        return 1; // No body
    } 
    int len = atol(lenstr);

    r->bodySize = len;
    r->body = malloc(len + 1);

    for (int i = 0; i < len; i++) {
        if (eof(stream)) {
            errno = BAD_REQUEST;
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
    if (!success) {
        freeHttpRequest(result);
        return NULL;
    }

    success = parseHeaders(result, stream);
    if (!success) {
        freeHttpRequest(result);
        return NULL;
    }

    success = parseBody(result, stream);
    if (!success) {
        freeHttpRequest(result);
        return NULL;
    }

    return result;
}

Headers createDefaultHeaders(int contentLength, char *contentType) {
    int defaultSize = 2 + (contentLength < 0 ? 0 : 1);
    Headers hs = { calloc(defaultSize, sizeof(Headers)), defaultSize };

    int i = 0;
    hs.entries[i++] = (Header) { strdup("Connection"), strdup("close") };
    hs.entries[i++] = (Header) { strdup("Content-Type"), strdup(contentType != NULL ? contentType : "text/plain") };

    if (contentLength >= 0) {
        char buf[16];
        sprintf(buf, "%d", contentLength);
        hs.entries[i++] = (Header) { strdup("Content-Length"), strdup(buf) };
    }

    return hs;
}

HttpResponse *ok_empty() {
    return ok_text(NULL);
}

HttpResponse *ok_text(char *text) {
    if (text == NULL) {
        return ok_body(NULL, text, 0);
    } else {
        return ok_body("text/plain", text, strlen(text));
    }
}

HttpResponse *ok_body(char *type, char *body, int len) {
    return ok((Headers){ NULL, 0 }, type, body, len);
}

HttpResponse *ok_file(char *type, char *path) {
    FILE *f = fopen(path, "r");
    if (f == NULL) {
        printf("File '%s' doesn't exist\n", path);
        return NULL;
    }

    fseek(f, 0, SEEK_END);
    int len = ftell(f);
    rewind(f);
    char *res = malloc(len + 1);

    for (int i = 0; i < len; i++) {
        res[i] = fgetc(f);
    }

    fclose(f);
    return ok_body(type, res, len);
}

HttpResponse *ok_chunked(int chunkSize, char *type, char *path) {
    HttpResponse *r = calloc(1, sizeof(HttpResponse));
    r->code = OK;
    r->headers = createDefaultHeaders(-1, type);     
    addHeader(&r->headers, "Transfer-Encoding", "chunked");

    r->bodyLocation = strdup(path);
    r->chunkSize = chunkSize;

    return r;
}

HttpResponse *ok(Headers headers, char *type, char *body, int len) {
    HttpResponse *r = calloc(1, sizeof(HttpResponse));
    r->code = OK;
    r->headers = createDefaultHeaders(len, type);     

    if (headers.length != 0) {
        r->headers.length += headers.length;
        r->headers.entries = realloc(r->headers.entries, r->headers.length);
        memcpy(r->headers.entries + (r->headers.length - headers.length - 1), headers.entries, headers.length);
        free(headers.entries);
    }

    if (len > 0) {
        r->body = malloc(len);
        memcpy(r->body, body, len);
        r->bodySize = len;
    }

    return r;
}

HttpResponse *badRequest() {
    HttpResponse *r = calloc(1, sizeof(HttpResponse));
    r->code = BAD_REQUEST;
    r->headers = createDefaultHeaders(0, NULL);
    return r;
}

char *getHeaderValue(Headers headers, char *key) {
    Header h;
    for (int i = 0; i < headers.length; i++) {
        h = headers.entries[i];
        if (!strcasecmp(h.key, key)) {
            return h.value;
        }
    }

    return NULL;
}

void addHeader(Headers *headers, char *key, char *val) {
    headers->entries = realloc(headers->entries, (++headers->length) * sizeof(Header));
    headers->entries[headers->length - 1] = (Header) { strdup(key), strdup(val) };
}

void freeHttpRequest(HttpRequest *r) {
    free(r->target);
    free(r->httpVersion);
    free(r->body);

    for (int i = 0; i < r->headers.length; i++) {
        Header h = r->headers.entries[i];
        free(h.key);
        free(h.value);
    }
    free(r->headers.entries);

    free(r);
}

void freeHttpResponse(HttpResponse *r) {
    free(r->body);
    free(r->bodyLocation);

    for (int i = 0; i < r->headers.length; i++) {
        Header h = r->headers.entries[i];
        free(h.key);
        free(h.value);
    }
    free(r->headers.entries);

    free(r);
}

