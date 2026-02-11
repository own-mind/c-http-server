#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "http.h"
#include "charStream.h"
#include "examples/qrcode/qrcode.h"

void assertNotNull(void *p) {
    if (p == NULL) {
        printf("\e[0;31mTest failed. Unexpected null (errno=%d).\e[0m\n", errno);
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
    char *v = getHeaderValue(r->headers, key);
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

void testHttpParsing() {
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
    assertInt(0, r->headers.length);
    freeHttpRequest(r);
    freeStream(stream);

    printf("Test #%d: Parsing Corrupted Request Line\n", ++testCount);
    stream = createStringStream("/coffee HTTP/1.1\r\nHost: localhost:42069\r\nUser-Agent: curl/7.81.0\r\nAccept: */*\r\n\r\n");
    r = parseHttpRequest(stream);
    assert(r == NULL, "Http request parsing should return null");
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
    assert(r == NULL, "Http request parsing should return null");
    freeStream(stream);

    printf("Test #%d: Header Key Constrains\n", ++testCount);
    stream = createStringStream("GET /coffee HTTP/1.1\r\nHost-Post-mOsT: localhost:42069\r\nShould-!#$%&'*+-.^_`|~0123456789-Allowed: Value\r\n\r\n");
    r = parseHttpRequest(stream);
    assertNotNull(r);
    assertHeader(r, "Host-Post-mOsT", "localhost:42069");
    assertHeader(r, "host-post-most", "localhost:42069");
    assertHeader(r, "hOst-pOST-mOSt", "localhost:42069");
    assertHeader(r, "Should-!#$%&'*+-.^_`|~0123456789-Allowed", "Value");
    freeHttpRequest(r);
    freeStream(stream);

    printf("Test #%d: Header Invalid Key #1\n", ++testCount);
    stream = createStringStream("GET /coffee HTTP/1.1\r\nОшибка: Value\r\n\r\n");
    r = parseHttpRequest(stream);
    assert(r == NULL, "Http request parsing should return null");
    freeStream(stream);

    printf("Test #%d: Header Invalid Key #2\n", ++testCount);
    stream = createStringStream("GET /coffee HTTP/1.1\r\nError Error: Value\r\n\r\n");
    r = parseHttpRequest(stream);
    assert(r == NULL, "Http request parsing should return null");
    freeStream(stream);

    printf("Test #%d: Duplicate Key Headers\n", ++testCount);
    stream = createStringStream("GET /coffee HTTP/1.1\r\nTest: 1\r\nTest: 2\r\nTest: 3\r\n\r\n");
    r = parseHttpRequest(stream);
    assertNotNull(r);
    assertHeader(r, "Test", "1,2,3");
    freeHttpRequest(r);
    freeStream(stream);

    printf("Test #%d: Parsing Full Request\n", ++testCount);
    stream = createStringStream("GET / HTTP/1.1\r\nHost: localhost:42069\r\nUser-Agent: curl/7.81.0\r\nAccept: */*\r\nContent-Length: 14\r\n\r\nBody body body");
    r = parseHttpRequest(stream);
    assertNotNull(r);
    assertInt(GET, r->method);
    assertString("/", r->target);
    assertString("1.1", r->httpVersion);
    assertHeader(r, "Host", "localhost:42069");
    assertHeader(r, "User-Agent", "curl/7.81.0");
    assertHeader(r, "Accept", "*/*");
    assertHeader(r, "Content-Length", "14");
    assertString("Body body body", r->body);
    assertInt(14, r->bodySize);
    freeHttpRequest(r);
    freeStream(stream);
}

void testQrModeSelection() {
    int testCount = 0;
    char *data;
    ModeGroup result;

    printf("Test #%d: Full numeric\n", ++testCount);
    data = "12345";
    result = selectMode(data, strlen(data));
    assertInt(NUMERIC, result.mode);
    assertInt(5, result.length);  

    printf("Test #%d: Start numeric\n", ++testCount);
    data = "1234567abcd";
    result = selectMode(data, strlen(data));
    assertInt(NUMERIC, result.mode);
    assertInt(7, result.length);

    printf("Test #%d: Full alphanum (no numbers)\n", ++testCount);
    data = "abcdfe$+-";
    result = selectMode(data, strlen(data));
    assertInt(ALPHANUMERIC, result.mode);
    assertInt(strlen(data), result.length);

    printf("Test #%d: Full alphanum (inner numbers)\n", ++testCount);
    data = "abcd123fe$+-";
    result = selectMode(data, strlen(data));
    assertInt(ALPHANUMERIC, result.mode);
    assertInt(strlen(data), result.length);

    printf("Test #%d: Start alphanum (before numbers)\n", ++testCount);
    data = "abcd1234567890fe$+-";   // Here it is better to encode 1234567890 as NUMERIC
    result = selectMode(data, strlen(data));
    assertInt(ALPHANUMERIC, result.mode);
    assertInt(4, result.length);

    printf("Test #%d: All bytes\n", ++testCount);
    data = "\1\2\3\4\5";
    result = selectMode(data, strlen(data));
    assertInt(BYTE, result.mode);
    assertInt(5, result.length);

    printf("Test #%d: Alpha before bytes\n", ++testCount);
    data = "abc\ncdcddd";
    result = selectMode(data, strlen(data));
    assertInt(ALPHANUMERIC, result.mode);
    assertInt(3, result.length);

    printf("Test #%d: Alpha after bytes\n", ++testCount);
    data = "\ncdcdddd";
    result = selectMode(data, strlen(data));
    assertInt(BYTE, result.mode);
    assertInt(1, result.length);
}

void assertBitBuffer(byte *bitBuffer, int bn, char *data) {
    int n = strlen(data);
    char *filtered = malloc(n + 1);
    int fi = 0;
    for (int i = 0; i < n; i++) {
        if (data[i] == '0' || data[i] == '1') {
            filtered[fi++] = data[i];
        }
    }
    filtered[fi] = '\0';

    n = strlen(filtered);
    char *bitString = malloc(bn + 1);

    for (int i = 0; i < bn; i++) {
        if (bitBuffer[i]) bitString[i] = '1';
        else              bitString[i] = '0';
    }
    bitString[bn] = '\0';


    assertString(filtered, bitString);
    free(bitString);
    free(filtered);
}

void testEncodings() {
    int testCount = 0;
    char *data;
    byte *bitBuffer = calloc(100, sizeof(byte));
    int bi;

    printf("Test #%d: Numeric string '12345'\n", ++testCount);
    bi = 0;
    data = "12345";
    encodeNumeric(bitBuffer, &bi, data, strlen(data));
    assertBitBuffer(bitBuffer, bi, "0001111011 0101101");

    printf("Test #%d: Numeric string '1'\n", ++testCount);
    bi = 0;
    memset(bitBuffer, 1, 100);
    data = "1";
    encodeNumeric(bitBuffer, &bi, data, strlen(data));
    assertBitBuffer(bitBuffer, bi, "0001");

    printf("Test #%d: Numeric string '00'\n", ++testCount);
    bi = 0;
    data = "00";
    encodeNumeric(bitBuffer, &bi, data, strlen(data));
    assertBitBuffer(bitBuffer, bi, "0000000");
    printf("Test #%d: Numeric string '000'\n", ++testCount);

    bi = 0;
    data = "000";
    encodeNumeric(bitBuffer, &bi, data, strlen(data));
    assertBitBuffer(bitBuffer, bi, "0000000000");

    printf("Test #%d: Numeric string '00000'\n", ++testCount);
    bi = 0;
    data = "00000";
    encodeNumeric(bitBuffer, &bi, data, strlen(data));
    assertBitBuffer(bitBuffer, bi, "0000000000 0000000");

    printf("Test #%d: Numeric string '511511511311'\n", ++testCount);
    bi = 0;
    data = "511511511311";
    encodeNumeric(bitBuffer, &bi, data, strlen(data));
    assertBitBuffer(bitBuffer, bi, "0111111111 0111111111 0111111111 0100110111");

    printf("Test #%d: Numeric string '01234567'\n", ++testCount);
    bi = 0;
    data = "01234567";
    encodeNumeric(bitBuffer, &bi, data, strlen(data));
    assertBitBuffer(bitBuffer, bi, "0000001100 0101011001 1000011");

    free(bitBuffer);
}

void assertArray(byte *expected, byte *actual, int n) {
    for (int i = 0; i < n; i++) {
        if (actual[i] != expected[i]) {
            printf("\e[0;31mTest failed.\nActual:\n");

            byte *arr = actual;
            printf("{ %d",(int)arr[0]);
            for (int i = 1; i < n; i++) {
                printf(", %d", (int)arr[i]);
            }

            printf(" }\n\nExpected:\n");

            arr = expected;
            printf("{ %d", (int)arr[0]);
            for (int i = 1; i < n; i++) {
                printf(", %d", (int)arr[i]);
            }

            printf(" }\e[0m\n");
            exit(1);
        }
    }
}

void testEC() {
    int testCount = 0;
    byte buffer[44];
    int n;

    printf("Test #%d: Basic example\n", ++testCount);
    byte data1[] = { 32,91,11,120,209,114,220,77,67,64,236,17,236,17,236,17 };
    byte result1[] = { 196,35,39,119,235,215,231,226,93,23 };
    n = generateEC(data1, sizeof(data1), buffer, 10, EC_GEN_L);
    assertArray(result1, buffer, n);
}

void testDataPack() {
    int testCount = 0;
    byte **matrix = malloc(25 * sizeof(byte*));
    for (int y = 0; y < 25; y++) {
        matrix[y] = calloc(25, sizeof(byte));
    }

    printf("Test #%d: Data Pack\n", ++testCount);
    byte data[44];
    for (int i = 0; i < 44; i++) data[i] = i + 1;

    byte result[25][25] = {
        { 0,0,0,0,0,0,0,0,0,0,1,1,1,0,1,1,0,0,0,0,0,0,0,0,0 },
        { 0,0,0,0,0,0,0,0,0,1,0,1,1,0,0,0,1,0,0,0,0,0,0,0,0 },
        { 0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,0,0,0,0,0,0,0,0,0,0 },
        { 0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,0,0,0,0,0,0,0,0,0,0 },
        { 0,0,0,0,0,0,0,0,0,0,0,1,1,0,0,1,0,0,0,0,0,0,0,0,0 },
        { 0,0,0,0,0,0,0,0,0,1,0,1,1,0,0,0,1,0,0,0,0,0,0,0,0 },
        { 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0 },
        { 0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,0,0,0,0,0,0,0,0,0,0 },
        { 0,0,0,0,0,0,0,0,0,0,0,0,1,0,1,0,1,0,0,0,0,0,0,0,0 },
        { 0,1,1,0,0,1,0,1,1,0,1,0,1,0,1,0,0,1,1,0,1,0,0,0,0 },
        { 1,0,1,0,1,0,0,0,0,1,0,1,1,0,0,0,1,0,0,0,0,0,0,1,0 },
        { 1,0,1,0,1,0,0,1,0,0,0,0,0,0,1,0,0,0,0,0,0,1,0,0,0 },
        { 0,1,0,0,0,0,0,0,0,1,0,0,0,1,1,0,0,1,1,0,1,1,0,0,0 },
        { 0,0,1,0,0,0,0,1,1,0,0,0,1,0,0,0,0,0,0,0,1,0,0,1,1 },
        { 0,0,1,0,1,0,0,0,0,1,0,1,1,0,0,0,1,0,0,0,0,0,0,0,0 },
        { 0,0,1,0,1,0,0,1,0,0,0,0,0,0,1,0,0,0,0,0,0,1,0,0,0 },
        { 0,0,0,1,0,0,0,0,1,1,0,0,1,1,1,0,0,0,0,0,0,0,1,0,0 },
        { 0,0,0,0,0,0,0,0,0,0,1,1,0,0,1,1,0,0,0,0,0,0,0,0,1 },
        { 0,0,0,0,0,0,0,0,0,1,0,1,1,0,0,1,0,0,0,0,0,0,0,0,0 },
        { 0,0,0,0,0,0,0,0,0,0,0,0,0,1,1,1,0,0,0,0,0,1,0,0,0 },
        { 0,0,0,0,0,0,0,0,0,0,1,0,0,0,0,1,0,0,0,0,0,1,1,0,0 },
        { 0,0,0,0,0,0,0,0,0,0,0,1,0,0,0,0,0,1,1,1,0,0,0,1,0 },
        { 0,0,0,0,0,0,0,0,0,1,0,1,1,0,0,0,0,1,0,0,1,0,0,0,0 },
        { 0,0,0,0,0,0,0,0,0,0,0,0,0,1,1,0,1,0,0,0,0,0,1,0,0 },
        { 0,0,0,0,0,0,0,0,0,0,1,0,1,0,0,1,1,0,0,0,0,0,0,0,0 },
    };

    packData(matrix, data, 44);

    int match = 1;
    for (int y = 0; y < 25; y++) {
        for (int x = 0; x < 25; x++) {
            if (matrix[y][x] != result[y][x]) {
                match = 0;
                break;
            }
        }
    }

    if (!match) {
        printf("\e[0;31mTest failed. Displaying matrix diff:\e[0m\n");
        for (int y = 0; y < 25; y++) {
            for (int x = 0; x < 25; x++) {
                char c = matrix[y][x] ? '#' : 'O';
                if (matrix[y][x] != result[y][x]) {
                    printf("\e[0;31m%c\e[0m ", c);
                } else {
                    printf("%c ", c);
                }
            }
            printf("\n");
        }

        freeMatrix(matrix, 25);
        exit(1);
    }

    freeMatrix(matrix, 25);
}

int main() {
    printf("---Testing HTTP Request Parsing---\n");
    testHttpParsing();
    printf("\n");

    printf("---Testing QR-Code Encoding: Mode Selection---\n");
    testQrModeSelection();
    printf("\n");

    printf("---Testing QR-Code Encoding: Data Encodings---\n");
    testEncodings();
    printf("\n");

    printf("---Testing QR-Code Encoding: Error Correction---\n");
    testEC();
    printf("\n");

    printf("---Testing QR-Code Encoding: Test Data Packing---\n");
    testDataPack();
    printf("\n");

    printf("\e[0;32mAll test passed!\e[0m\n");
}
