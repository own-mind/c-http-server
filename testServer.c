#include <stdio.h>
#include "server.h"

HttpResponse *_index(HttpRequest *rq) {
    if (rq->bodySize > 0) {
        printf("Body: %s\n", rq->body);
    }
    return ok_file("examples/index.html");
}

int main() {
    Server *s = createServer();

    sget(s, "/", &_index);
    sserve(s, 42069);

    freeServer(s);
}
