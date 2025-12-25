#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include "http.h"

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

        CharStream *stream = createSocketStream(clientSockfd);
        HttpRequest *rq = parseHttpRequest(stream);

        printf("Request received:\n - Method: %d\n - Target: %s\n - HTTP version: %s\n", rq->method, rq->target, rq->httpVersion);

        freeStream(stream);
        freeHttpRequest(rq);

        shutdown(clientSockfd, 2);
        close(clientSockfd);
    }

    close(sockfd);
}
