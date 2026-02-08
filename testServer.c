#include <stdio.h>
#include <stdlib.h>
#include "server.h"
#include "examples/qrcode/qrcode.h"

HttpResponse *_index(HttpRequest *rq, char **ms, size_t n) {
    if (rq->bodySize > 0) {
        printf("Body: %s\n", rq->body);
    }

    return ok_file("text/html", "examples/index.html");
}

double extractNumber(int *index, char *s, int n) {
    char *buf = malloc(n);
    int i = 0;
    
    while (*index < n) {
        char c = s[(*index)++];
        if (c == '\n') break;

        buf[i++] = c;
    }
    buf[i] = '\0';

    double res =atof(buf);
    free(buf);
    return res;
}

HttpResponse *_calculate(HttpRequest *rq, char **matches, size_t matchesLen) {
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

HttpResponse *_chunked(HttpRequest *rq, char **matches, size_t matchesLen) {
    int amount = atoi(matches[0]);

    HttpResponse *r = ok_chunked(amount, "image/jpeg", "examples/cat.jpg");
    addHeader(&r->trailers, "Test-Trailer", "12345");

    return r;
}

HttpResponse *_qrpage(HttpRequest *rq, char **matches, size_t matchesLen) {
    return ok_file("text/html", "examples/qrcode/qr.html");
}

HttpResponse *_qrgen(HttpRequest *rq, char **matches, size_t matchesLen) {
    QRCode *qr = generateQR(rq->body, rq->bodySize);
    if (qr == NULL) return badRequest();

    HttpResponse* resp = ok_body("image/bmp", (char *) qr->data, qr->len);
    freeQR(qr);
    return resp;
}

int main() {
    Server *s = createServer();

    sget(s, "/", &_index);
    spost(s, "/calculate/", &_calculate);
    sget(s, "/chunked/([[:digit:]]+)", &_chunked);
    spost(s, "/qrgen/", &_qrgen);
    sget(s, "/qr/", &_qrpage);
    sserve(s, 42069);

    freeServer(s);
}
