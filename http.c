#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "http.h"

LineArray getLines(int clientSockfd) {
    char line[256];
    char **result = malloc(0);
    int n = 0;
    
    size_t lineSize;
    char *resultLine;
    char readByte;
    int i;
    ssize_t recvSize = 1;
    while (recvSize > 0) {
        int i = 0;
        while (i < sizeof(line) && (recvSize = recv(clientSockfd, &readByte, 1, 0)) > 0) {
            line[i++] = readByte;
            if (readByte == '\n') break;
        }
        if (i == 0) break;
        line[i] = '\0';
        result = realloc(result, ++n * sizeof(char*));

        lineSize = strlen(line);
        resultLine = malloc((lineSize + 1) * sizeof(char));
        strncpy(resultLine, line, lineSize);

        result[n - 1] = resultLine;
    }
    
    LineArray arr = { result, n } ;
    return arr;
}

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

HttpRequest *parseHttpRequest(CharStream *stream) {
    HttpRequest *result = calloc(1, sizeof(HttpRequest));

    int success = parseRequestLine(result, stream);
    if (!success) return NULL;

    return result;
}

void freeHttpRequest(HttpRequest *r) {
    free(r->target);
    free(r->httpVersion);
    free(r->body);
    free(r);
}

void freeLineArray(LineArray arr) {
    for (int i = 0; i < arr.size; i++) {
        free(arr.lines[i]);
    }
    free(arr.lines);
}

int testSocket() {
    int sockfd;
    struct sockaddr_in servAddr;

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        perror("Unable to create socket");
        return 1;
    }

    servAddr.sin_family = AF_INET;
    servAddr.sin_port = htons(42069);
    inet_pton(AF_INET, "127.0.0.1", &servAddr.sin_addr);

    bind(sockfd, (struct sockaddr*) &servAddr, sizeof(servAddr));
    if (listen(sockfd, 1) < 0) {
        perror("Unable to listen");
        return 1;
    }
    
    while (1) {
        struct sockaddr clientAddr;
        socklen_t clientAddrLen;
        int clientSockfd = accept(sockfd, (struct sockaddr *) &clientAddr, &clientAddrLen);

        LineArray arr = getLines(clientSockfd);
        for (int i = 0; i < arr.size; i++) {
            printf("read: %s\n", arr.lines[i]);
            free(arr.lines[i]);
        }
        free(arr.lines);

        close(clientSockfd);
    }

    close(sockfd);
}
