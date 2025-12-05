#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
    char **lines;
    int size;
} LineArray;

LineArray getLines(int clientSockfd) {
    FILE *file = fdopen(clientSockfd, "r");
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
