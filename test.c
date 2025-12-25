#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "http.h"
#include "charStream.h"

void assertNotNull(void *p) {
    if (p == NULL) {
        printf("\e[0;31mTest failed. Unexpected null\e[0m\n");
        exit(1);
    }
}

void assertString(char* expected, char* actual) {
    if (strcmp(actual, expected) != 0) {
        printf("\e[0;31mTest failed.\nActual:\n%s\n", actual);

        char *t = actual;
        while (*t != '\0') {
            printf("%d ", *t);
            t++;
        }
        
        printf("\n\nExpected:\n%s\n\n", expected);

        t = expected;
        while (*t != '\0') {
            printf("%d ", *t);
            t++;
        }
        printf("\e[0m\n");
        exit(1);
    }
}

void assertInt(int expected, int actual) {
    if (expected != actual) {
        printf("\e[0;31mTest failed.\nActual:\n%d\n\nExpected:\n%d\e[0m\n", actual, expected);
        exit(1);
    }
}

// Looks for header with provided key and value
void assertHeader(HttpRequest *r, char *key, char *value) {
    char *v = getHeaderValue(r, key);
    if (v == NULL) {
        printf("\e[0;31mTest failed.\nExpected header '%s: %s', but it doesn't exists\e[0m\n", key, value);
        exit(1);
    }

    assertString(value, v);
}

void assert(int condition, char *message) {
    if (!condition) {
        printf("\e[0;31mTest failed: %s\e[0m\n", message);
        exit(1);
    }
}

int main() {
    int testCount = 0;
    CharStream *stream;
    HttpRequest *r;

    printf("Test #%d: Parsing Request Line\n", ++testCount);
    stream = createStringStream("GET /coffee HTTP/1.1\r\n\r\n");
    r = parseHttpRequest(stream);
    assertNotNull(r);
    assertInt(GET, r->method);
    assertString("/coffee", r->target);
    assertString("1.1", r->httpVersion);
    assertInt(0, r->headersSize);
    freeHttpRequest(r);
    freeStream(stream);

    printf("Test #%d: Parsing Corrupted Request Line\n", ++testCount);
    stream = createStringStream("/coffee HTTP/1.1\r\nHost: localhost:42069\r\nUser-Agent: curl/7.81.0\r\nAccept: */*\r\n\r\n");
    r = parseHttpRequest(stream);
    assert(r == NULL, "Http request should return null");
    freeStream(stream);

    printf("Test #%d: Parsing Goofy Header\n", ++testCount);
    stream = createStringStream("GET /coffee HTTP/1.1\r\n        Host:   localhost:42069     \r\n\r\n");
    r = parseHttpRequest(stream);
    assertNotNull(r);
    assertHeader(r, "Host", "localhost:42069");
    freeHttpRequest(r);
    freeStream(stream);

    printf("Test #%d: Parsing Corrupted Header\n", ++testCount);
    stream = createStringStream("GET /coffee HTTP/1.1\r\n        Host : localhost:42069     \r\n\r\n");
    r = parseHttpRequest(stream);
    assert(r == NULL, "Http request should return null");
    freeStream(stream);

    printf("Test #%d: Parsing Full Request\n", ++testCount);
    stream = createStringStream("GET / HTTP/1.1\r\nHost: localhost:42069\r\nUser-Agent: curl/7.81.0\r\nAccept: */*\r\n\r\n");
    r = parseHttpRequest(stream);
    assertNotNull(r);
    assertInt(GET, r->method);
    assertString("/", r->target);
    assertString("1.1", r->httpVersion);
    assertHeader(r, "Host", "localhost:42069");
    assertHeader(r, "User-Agent", "curl/7.81.0");
    assertHeader(r, "Accept", "*/*");
    freeHttpRequest(r);
    freeStream(stream);

    printf("\e[0;32mAll test passed!\e[0m\n");
}
