#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <signal.h>
#include "server.h"

volatile sig_atomic_t run_flag = 1;

typedef struct {
    RequestMethod method;
    RequestHandler handler;
    regex_t expression;
} Route;

struct Server {
    Route **routes;
    int routesLen;
};

Server *createServer() {
    Server *server = calloc(1, sizeof(Server));
    return server;
}

void sget(Server *s, char *pattern, RequestHandler handler) {
    sadd(s, pattern, handler, GET);
}

void spost(Server *s, char *pattern, RequestHandler handler) {
    sadd(s, pattern, handler, POST);
}

void sadd(Server *s, char *pattern, RequestHandler handler, RequestMethod m) {
    Route *route = calloc(1, sizeof(Route));
    route->handler = handler;
    route->method = m;

    int pn = strlen(pattern);
    if(pattern[0] != '^' || pattern[pn - 1] != '&') {
        char *start = pattern[0] == '^' ? "" : "^";
        char *end = pattern[pn - 1] == '$' ? "" : "$";

        char *new;
        asprintf(&new, "%s%s%s", start, pattern, end);
        pattern = new;
    }

    if (regcomp(&route->expression, pattern, REG_EXTENDED) != 0) {
       fprintf(stderr, "Invalid pattern: %s\n", pattern); 
       exit(1);
    }

    s->routes = realloc(s->routes, ++s->routesLen * sizeof(Route));
    s->routes[s->routesLen - 1] = route;
}

Route *findRoute(Server *s, HttpRequest *rq, char ***matchesOut) {
    for (int r = 0; r < s->routesLen; r++) {
        Route *route = s->routes[r];
        if(rq->method != route->method) continue;

        regmatch_t *matches = malloc((route->expression.re_nsub + 1) * sizeof(regmatch_t));
        *matchesOut = route->expression.re_nsub == 0 ? NULL : malloc(route->expression.re_nsub * sizeof(char*));

        if (regexec(&route->expression, rq->target, route->expression.re_nsub + 1, matches, 0) == 0) {
            for (size_t i = 0; i < route->expression.re_nsub; i++) {
                regmatch_t match = matches[i + 1];
                if (match.rm_so == -1) {
                    (*matchesOut)[i] = NULL;
                    continue;
                }

                int len = (int)(match.rm_eo - match.rm_so);
                char *group = malloc(len + 1);
                memcpy(group, rq->target + match.rm_so, len);
                group[len] = '\0';

                (*matchesOut)[i] = group;
            }

            free(matches);
            return route;
        }

        free(matches);
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

int compileResponse(HttpResponse *r, char **resultOut, char **trailersString) {
    // Declaring trailers
    if (r->trailers.length > 0) {
        *trailersString = malloc(1);
        *trailersString[0] = '\0';

        char *trailerHeader = NULL;
        char *old;

        for (int i = 0; i < r->trailers.length; i++) {
            Header *h = r->trailers.entries + i;

            old = *trailersString;
            asprintf(trailersString, "%s%s: %s\r\n", old, h->key, h->value);
            free(old);

            if (trailerHeader == NULL) {
                trailerHeader = strdup(h->key);
            } else {
                old = trailerHeader;
                asprintf(&trailerHeader, "%s, %s", old, h->key);
                free(old);
            }
        }

        addHeader(&r->headers, "Trailer", trailerHeader);
        free(trailerHeader); // It is strduped
    } else {
        trailersString = NULL;
    }

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

    int rn = strlen(result);
    if (r->bodySize) {
        result = realloc(result, rn + r->bodySize + 1);
        memcpy(result + rn, r->body, r->bodySize);
        result[rn + r->bodySize] = '\0';
    }
    
    *resultOut = result;
    return rn + r->bodySize;
}

void sendStatusResponse(StatusCode code, int clientSockfd) {
    HttpResponse *r = calloc(1, sizeof(HttpResponse));
    r->code = code;
    char *rs;
    char *ignore;
    int rn = compileResponse(r, &rs, &ignore);
    write(clientSockfd, rs, rn); 
    free(rs);
    free(r);
}

void sendChunked(HttpResponse *response, int clientSockfd) {
    int chunkSize = response->chunkSize;
    FILE *f = fopen(response->bodyLocation, "r");
    if (f == NULL) {
        perror("Error opening file");
        exit(1);
    }

    char *buffer = malloc(chunkSize);
    int bytesRead;
    char chunkSizeHex[32];
    int cn;

    while(!feof(f)) {
        bytesRead = fread(buffer, 1, chunkSize, f);

        cn = snprintf(chunkSizeHex, 32, "%x\r\n", bytesRead);
        write(clientSockfd, chunkSizeHex, cn);
        write(clientSockfd, buffer, bytesRead);
        write(clientSockfd, "\r\n", 2);
    }

    write(clientSockfd, "0\r\n", 3);  // Finishing chunked

    free(buffer);
    fclose(f);
}

void sig_handler(int signum) {
    if (signum == SIGINT || signum == SIGTERM) {
        run_flag = 0;
    }
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

    struct sigaction sa = {0};
    sa.sa_handler = sig_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;   // IMPORTANT: no SA_RESTART

    sigaction(SIGTERM, &sa, NULL);
    sigaction(SIGINT,  &sa, NULL);

    while (run_flag) {   //TODO make multi-threaded
        struct sockaddr clientAddr;
        socklen_t clientAddrLen;
        int clientSockfd = accept(sockfd, (struct sockaddr *) &clientAddr, &clientAddrLen);

        if (clientSockfd == -1) {
            if (errno == EINTR) break;
            continue;
        }

        CharStream *stream = createSocketStream(clientSockfd);
        errno = 0;
        HttpRequest *rq = parseHttpRequest(stream);

        if (rq == NULL) {
            sendStatusResponse(errno > 0 ? (StatusCode) errno : BAD_REQUEST, clientSockfd);
        } else {
            char **matches = NULL; 
            Route *route = findRoute(server, rq, &matches);
            if(route != NULL) {
                HttpResponse *response = route->handler(rq, matches, route->expression.re_nsub);

                for (size_t i = 0; i < route->expression.re_nsub; i++) {
                    free(matches[i]);
                }
                free(matches);

                if (response != NULL) {
                    char *responseString;
                    char *trailersString = NULL;
                    int rn = compileResponse(response, &responseString, &trailersString);
                    write(clientSockfd, responseString, rn);
                    free(responseString);

                    if (response->bodyLocation != NULL) {  // Assuming chunked connection
                        sendChunked(response, clientSockfd);
                    }

                    if (trailersString != NULL) {
                        write(clientSockfd, trailersString, strlen(trailersString));
                        free(trailersString);
                    }

                    if (response->bodyLocation != NULL) {
                        write(clientSockfd, "\r\n", 2);
                    }

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

void freeRoute(Route *r) {
    regfree(&r->expression);
    free(r);
}

void freeServer(Server *s) {
    for (int i = 0; i < s->routesLen; i++) {
        freeRoute(s->routes[i]);
    }
    free(s->routes);
    free(s);
}
