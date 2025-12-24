#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "http.h"
#include "charStream.h"

void assertNotNull(void *p) {
    if (p == NULL) {
        printf("Test failed. Unexpected null\n");
        exit(1);
    }
}

void assertString(char* expected, char* actual) {
    if (strcmp(actual, expected) != 0) {
        printf("Test failed.\nActual:\n%s\n", actual);

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
        printf("\n");
        exit(1);
    }
}

void assertInt(int expected, int actual) {
    if (expected != actual) {
        printf("Test failed.\nActual:\n%d\n\nExpected:\n%d\n", actual, expected);
        exit(1);
    }
}

void assert(int condition, char *message) {
    if (!condition) {
        printf("Test failed: %s\n", message);
        exit(1);
    }
}

int main() {
    CharStream *stream;
    HttpRequest *r;

    printf("Test #1\n");
    stream = createStringStream("GET / HTTP/1.1\r\nHost: localhost:42069\r\nUser-Agent: curl/7.81.0\r\nAccept: */*\r\n\r\n");
    r = parseHttpRequest(stream);
    assertNotNull(r);
    assertInt(GET, r->method);
    assertString("/", r->target);
    assertString("1.1", r->httpVersion);
    freeHttpRequest(r);
    freeStream(stream);

    printf("Test #2\n");
    stream = createStringStream("GET /coffee HTTP/1.1\r\nHost: localhost:42069\r\nUser-Agent: curl/7.81.0\r\nAccept: */*\r\n\r\n");
    r = parseHttpRequest(stream);
    assertNotNull(r);
    assertInt(GET, r->method);
    assertString("/coffee", r->target);
    assertString("1.1", r->httpVersion);
    freeHttpRequest(r);
    freeStream(stream);

    printf("Test #3\n");
    stream = createStringStream("/coffee HTTP/1.1\r\nHost: localhost:42069\r\nUser-Agent: curl/7.81.0\r\nAccept: */*\r\n\r\n");
    r = parseHttpRequest(stream);
    assert(r == NULL, "Http request should return null");
    freeStream(stream);

    printf("All test passed!\n");
}
