#ifndef __SERVER_H
#define __SERVER_H

#include "http.h"

typedef HttpResponse *(*RequestHandler)(HttpRequest* request);
struct Server;
typedef struct Server Server;

Server *createServer();
void sget(Server *s, char *path, RequestHandler handler);
void spost(Server *s, char *path, RequestHandler handler);
void sadd(Server *s, char *path, RequestHandler handler, RequestMethod m);
void sserve(Server *s, int port);
void freeServer(Server *s);

#endif
