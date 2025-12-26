#ifndef __HTTP_H
#define __HTTP_H

#include <errno.h>
#include "charStream.h"

typedef struct {
    char **lines;
    int size;
} LineArray;

void freeLineArray(LineArray r);

typedef enum {
  GET, POST, DELETE, PUT, CONNECT, PATCH, TRACE, HEAD, OPTIONS
} RequestMethod;

typedef struct {
  char *key;
  char *value;
} Header;

typedef struct  {
  RequestMethod method;
  char *target;
  char *httpVersion;

  Header *headers;
  int headersSize;

  char *body;
  long bodySize;
} HttpRequest;

/**
 * Looks for value of a header with provided key, or null of not found.
 */
char *getHeaderValue(HttpRequest *r, char *key);

void freeHttpRequest(HttpRequest *r);

HttpRequest *parseHttpRequest(CharStream *stream);

#endif
