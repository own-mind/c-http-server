#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "server.h"

typedef struct {
    RequestHandler handler;
    char *path;
    RequestMethod method;
} Route;

struct Server {
    Route **routes;
    int routesLen;
};

Server *createServer() {
    Server *server = calloc(1, sizeof(Server));
    return server;
}

void sget(Server *s, char *path, RequestHandler handler) {
    sadd(s, path, handler, GET);
}

void spost(Server *s, char *path, RequestHandler handler) {
    sadd(s, path, handler, POST);
}

void sadd(Server *s, char *path, RequestHandler handler, RequestMethod m) {
    //TODO would be better to write a parser for path and a validator

    Route *route = calloc(1, sizeof(Route));
    route->handler = handler;
    route->path = path;
    route->method = m;

    s->routes = realloc(s->routes, ++s->routesLen * sizeof(Route));
    s->routes[s->routesLen - 1] = route;
}

Route *findRoute(Server *s, HttpRequest *rq) {
    for (int i = 0; i < s->routesLen; i++) {
        Route *route = s->routes[i];

        if(rq->method == route->method && !strcmp(route->path, rq->target)) {
            return route;
        }
    }

    return NULL;
}

char *statusMessage(StatusCode code) {
    switch (code) {
        case CONTINUE: return "Continue";
        case SWITCHING_PROTO: return "Switching Protocol";
        case PROCESSING: return "Processing";
        case EARLY_HINTS: return "Early Hints";

        case OK: return "Ok";
        case CREATED: return "Created";
        case ACCEPTED: return "Accepted";
        case NON_AUTH_INFO: return "Non-Authoritative Information";
        case NO_CONTENT: return "No Content";
        case RESET_CONTENT: return "Reset Content";
        case PARTIAL_CONTENT: return "Partial Content";
        case MULTI_STATUS: return "Multi-Status";
        case ALREADY_REPORTED: return "Already Reported";
        case IM_USED: return "IM Used";

        case MULTIPLE_CHOICES: return "Multiple Choices";
        case MOVED_PERM: return "Moved Permanently";
        case FOUND: return "Found";
        case SEE_OTHER: return "See Other";
        case NOT_MODIFIED: return "Not Modified";
        case USE_PROXY: return "Use Proxy";
        case TEMPORARY_REDIRECT: return "Temporary Redirect";
        case PERMANENT_REDIRECT: return "Permanent Redirect";

        case BAD_REQUEST: return "Bad Request";
        case UNAUTHORIZED: return "Unauthorized";
        case PAYMENT_REQUIRED: return "Payment Required";
        case FORBIDDEN: return "Forbidden";
        case NOT_FOUND: return "Not Found";
        case METHOD_NOT_ALLOWED: return "Method Not Allowed";
        case NOT_ACCEPTABLE: return "Not Acceptable";
        case PROXY_AUTH_REQUIRED: return "Proxy Authentication Required";
        case REQUEST_TIMEOUT: return "Request Timeout";
        case CONFLICT: return "Conflict";
        case GONE: return "Gone";
        case LENGTH_REQUIRED: return "Length Required";
        case PRECONDITION_FAILED: return "Precondition Failed";
        case CONTENT_TOO_LARGE: return "Content Too Large";
        case URI_TOO_LONG: return "URI Too Long";
        case UNSUPPORTED_MEDIA_TYPE: return "Unsupported Media Type";
        case RANGE_NOT_SATISFIABLE: return "Range Not Satisfiable";
        case EXPECTATION_FAILED: return "Expectation Failed";
        case TEAPOT: return "I'm a teapot";
        case MISDIRECTED_REQUEST: return "Misdirected Request";
        case UNPROCESSABLE_CONTENT: return "Unprocessable Content";
        case LOCKED: return "Locked";
        case FAILED_DEPENDENCY: return "Failed Dependency";
        case TOO_EARLY: return "Too Early";
        case UPGRADE_REQUIRED: return "Upgrade Required";
        case PRECONDITION_REQUIRED: return "Precondition Required";
        case TOO_MANY_REQUEST: return "Too Many Request";
        case REQUEST_HEADER_FIELDS_TOO_LARGE: return "Request Header Fields Too Large";
        case UNAVAILABLE_FOR_LEGAL_REASONS: return "Unavailable For Legal Reasons";

        case INTERNAL_SERVER_ERROR: return "Internal Server Error";
        case NOT_IMPLEMENTED: return "Not Implemented";
        case BAD_GATEWAY: return "Bad Gateway";
        case SERVICE_UNAVAILABLE: return "Service Unavailable";
        case GATEWAY_TIMEOUT: return "Gateway Timeout";
        case HTTP_VERSION_NOT_SUPPORTED: return "Http Version Not Supported";
        case VARIANT_ALSO_NEGOTIATES: return "Variant Also Negotiates";
        case INSUFFICIENT_STORAGE: return "Insufficient Storage";
        case LOOP_DETECTED: return "Loop Detected";
        case NOT_EXTENDED: return "Not Extended";
        case NETWORK_AUTHENTICATION_REQUIRED: return "Network Authentication Require";
        default: return "Unknown Message";
    }
}

char *compileResponse(HttpResponse *r) {
    char *result;
    asprintf(&result, "HTTP/1.1 %d %s\r\n", r->code, statusMessage(r->code));
    
    for (int i = 0; i < r->headers.length; i++) {
        Header *h = r->headers.entries + i;

        char *old = result;
        asprintf(&result, "%s%s: %s\r\n", old, h->key, h->value);
        free(old);
    }

    char *old = result;
    asprintf(&result, "%s\r\n", old);
    free(old);

    if (r->bodySize) {
        int rn = strlen(result);
        result = realloc(result, rn + r->bodySize + 1);
        memcpy(result + rn, r->body, r->bodySize);
        result[rn + r->bodySize] = '\0';
    }
    
    return result;
}

void sendStatusResponse(StatusCode code, int clientSockfd) {
    HttpResponse *r = calloc(1, sizeof(HttpResponse));
    r->code = code;
    char *rs = compileResponse(r);
    write(clientSockfd, rs, strlen(rs));
    free(rs);
    free(r);
}

void sserve(Server *server, int port) {
    int sockfd;
    struct sockaddr_in servAddr;

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        perror("Unable to create socket");
        return;
    }

    servAddr.sin_family = AF_INET;
    servAddr.sin_port = htons(port);
    inet_pton(AF_INET, "127.0.0.1", &servAddr.sin_addr);

    bind(sockfd, (struct sockaddr*) &servAddr, sizeof(servAddr));
    if (listen(sockfd, 1) < 0) {
        perror("Unable to listen");
        return;
    }

    while (1) {   //TODO make multi-threaded
        struct sockaddr clientAddr;
        socklen_t clientAddrLen;
        int clientSockfd = accept(sockfd, (struct sockaddr *) &clientAddr, &clientAddrLen);

        CharStream *stream = createSocketStream(clientSockfd);
        errno = 0;
        HttpRequest *rq = parseHttpRequest(stream);

        if (rq == NULL) {
            sendStatusResponse(errno > 0 ? (StatusCode) errno : BAD_REQUEST, clientSockfd);
        } else {
            Route *route = findRoute(server, rq);
            if(route != NULL) {
                HttpResponse *response = route->handler(rq);

                if (response != NULL) {
                    char *responseString = compileResponse(response);
                    write(clientSockfd, responseString, strlen(responseString));
                    free(responseString);
                    free(response);
                } else {
                    sendStatusResponse(INTERNAL_SERVER_ERROR, clientSockfd);
                }
            } else {
                sendStatusResponse(NOT_FOUND, clientSockfd);
            }

            freeHttpRequest(rq);
        }

        freeStream(stream);

        shutdown(clientSockfd, 2);    //TODO
        close(clientSockfd);
    }

    close(sockfd);
}

void freeServer(Server *s) {
    for (int i = 0; i < s->routesLen; i++) {
        Route *r = s->routes[i];
        free(r->path);
    }
    free(s->routes);
    free(s);
}
