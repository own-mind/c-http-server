#include <stdio.h>
#include <stdlib.h>
#include "server.h"

HttpResponse *_index(HttpRequest *rq) {
    if (rq->bodySize > 0) {
        printf("Body: %s\n", rq->body);
    }

    return ok_file("text/html", "examples/index.html");
}

double extractNumber(int *index, char *s, int n) {
    char *buf = malloc(n);
    int i = 0;
    
    while (i < n) {
        char c = s[(*index)++];
        if (c == '\n') break;

        buf[i++] = c;
    }
    buf[i] = '\0';

    double res =atof(buf);
    free(buf);
    return res;
}

HttpResponse *_calculate(HttpRequest *rq) {
    if (rq->bodySize == 0) { printf("1\n"); return badRequest(); }

    int index = 0;
    double first = extractNumber(&index, rq->body, rq->bodySize);

    if (index >= rq->bodySize) return badRequest();
    
    char op = rq->body[index++];

    if (op != '+' && op != '-' && op != '*' && op != '/') return badRequest();
    if (index >= rq->bodySize || rq->body[index++] != '\n') return badRequest();

    double second = extractNumber(&index, rq->body, rq->bodySize);

    double result = first;
    switch (op) {
        case '+': result += second; break;
        case '-': result -= second; break;
        case '*': result *= second; break;
        case '/': result /= second; break;
        default: return badRequest();
    }

    char *resStr = NULL;
    asprintf(&resStr, "%f", result);
    return ok_text(resStr);
}

int main() {
    Server *s = createServer();

    sget(s, "/", &_index);
    spost(s, "/calculate/", &_calculate);
    sserve(s, 42069);

    freeServer(s);
}
